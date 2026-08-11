/*!
\file calculator.c
\brief TCP client that connects to the monitor, receives a range of
       integers to sum, and reports its progress back periodically.
       Renamed from the original processus.c to match what it actually
       implements (a "calculator", per the README's own terminology).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"
#include "structures.h"
#include "calculator.h"

/* State shared between partialSum() (writer) and sendSum() (reader) for
   a single calculator. Guarded by mutex; partialSum() never holds the
   lock across sleep(), so cancelling either thread here is always safe. */
typedef struct calc_state {
    pthread_mutex_t mutex;
    int sock;
    int partial_sum;
    int next_index;
    int end_index;
} calc_state;

/* Advances the running sum by one integer per second, so progress is
   visible to sendSum()/the monitor in near real time. */
static void *partialSum(void *arg) {
    calc_state *state = (calc_state *)arg;

    while (1) {
        pthread_mutex_lock(&state->mutex);
        int i = state->next_index;
        int end = state->end_index;
        if (i >= end) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }
        state->partial_sum += i;
        state->next_index = i + 1;
        pthread_mutex_unlock(&state->mutex);

        if (sleep(1) != 0) {
            die("sleep() of partialSum failed");
        }
    }
    pthread_exit(NULL);
}

/* Reports the current progress to the monitor every 2 seconds until the
   task range is exhausted, then sends one last report and returns. This
   is the thread the evil monkey targets to simulate a calculator that
   has stopped responding. */
static void *sendSum(void *arg) {
    calc_state *state = (calc_state *)arg;
    time_t start_time = time(NULL);
    int done = 0;

    while (!done) {
        progress_report report;

        pthread_mutex_lock(&state->mutex);
        report.partial_sum = state->partial_sum;
        report.next_index = state->next_index;
        done = (state->next_index >= state->end_index);
        pthread_mutex_unlock(&state->mutex);

        report.elapsed_time = time(NULL) - start_time;

        if (send(state->sock, &report, sizeof(report), 0) < 0) {
            die("send() of progress report failed");
        }

        if (!done && sleep(2) != 0) {
            die("sleep() of sendSum failed");
        }
    }
    pthread_exit(NULL);
}

void *CreateTCPClientSocket(void *arg) {
    calculator_param *params = (calculator_param *)arg;
    struct sockaddr_in monitor_addr;
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        die("socket() failed");
    }

    memset(&monitor_addr, 0, sizeof(monitor_addr));
    monitor_addr.sin_family = AF_INET;
    monitor_addr.sin_addr.s_addr = inet_addr(MONITOR_HOST);
    monitor_addr.sin_port = htons(MONITOR_PORT);

    if (connect(sock, (struct sockaddr *)&monitor_addr, sizeof(monitor_addr)) < 0) {
        die("connection to monitor failed");
    }

    task_assignment assignment;
    if (recv(sock, &assignment, sizeof(assignment), MSG_WAITALL) != (ssize_t)sizeof(assignment)) {
        die("failed to receive task assignment from monitor");
    }

    calc_state state;
    pthread_mutex_init(&state.mutex, NULL);
    state.sock = sock;
    state.partial_sum = assignment.partial_sum;
    state.next_index = assignment.begin_index;
    state.end_index = assignment.end_index;

    if (pthread_create(params->calcul_thread, NULL, partialSum, &state) != 0) {
        fprintf(stderr, "pthread_create error for partialSum thread\n");
    }
    if (pthread_create(params->report_thread, NULL, sendSum, &state) != 0) {
        fprintf(stderr, "pthread_create error for sendSum thread\n");
    }

    /* calcul_thread is joined first: once it is done, next_index has
       reached end_index, so sendSum's own loop condition will also be
       false and it will send its last report and return on its own. */
    pthread_join(*params->calcul_thread, NULL);
    pthread_join(*params->report_thread, NULL);

    pthread_mutex_destroy(&state.mutex);
    close(sock);

    pthread_exit(NULL);
}
