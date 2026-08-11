/*!
\file monitor.c
\brief TCP server that splits the [0, TASK_RANGE_MAX] summation across
       calculator clients, tracks their progress and reassigns any task
       whose calculator stops reporting.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"
#include "structures.h"
#include "thread_functions.h"
#include "monitor.h"

/* Prompts until the user enters a task count we can actually work with;
   the original version trusted raw scanf() output and could end up
   indexing the task array with a negative count on bad input. */
static int read_nb_processus(void) {
    int nb_processus_max = 0;

    while (1) {
        printf("Veuillez indiquer le nombre de processus que vous souhaitez utiliser (1-%d) : \n", MAX_CALCULATORS);

        int result = scanf("%d", &nb_processus_max);
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
            /* discard the rest of the line, including a bad token */
        }

        if (result == 1 && nb_processus_max > 0 && nb_processus_max <= MAX_CALCULATORS) {
            return nb_processus_max;
        }
        fprintf(stderr, "Merci d'entrer un entier entre 1 et %d.\n", MAX_CALCULATORS);
    }
}

static void print_report(int nb_processus_max, int global_sum, const task_cell *tasks) {
    printf("Nombre de processus calculateurs : %d\n", nb_processus_max);
    printf("Somme totale partielle calculee : %d\n", global_sum);
    for (int i = 0; i < nb_processus_max; i++) {
        printf("--PROCESSUS %d\n", i);
        printf("--somme partielle calculee : %d \n", tasks[i].partial_sum);
        printf("--Temps : %ld \n", (long)tasks[i].elapsed_time);
        printf("--Etat : %d \n", tasks[i].status);
    }
}

void *monitor(void *arg) {
    monitor_init_param *init = (monitor_init_param *)arg;
    volatile int *flag = init->flag;
    struct sockaddr_in monitor_addr;
    int sockfd;
    int global_sum = 0;
    pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

    int nb_processus_max = read_nb_processus();

    if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        die("socket() failed");
    }

    /* Lets the port be reused immediately after a previous run instead of
       sitting in TIME_WAIT and failing the next bind(). */
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&monitor_addr, 0, sizeof(monitor_addr));
    monitor_addr.sin_family = AF_INET;
    monitor_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    monitor_addr.sin_port = htons(MONITOR_PORT);

    if (bind(sockfd, (struct sockaddr *)&monitor_addr, sizeof(monitor_addr)) < 0) {
        die("bind() failed");
    }
    if (listen(sockfd, nb_processus_max) < 0) {
        die("listen() failed");
    }

    /* We are now ready to accept calculators: let the launcher know so it
       stops waiting and starts offering to spawn them. */
    sem_post(init->ready);

    task_cell *tasks = malloc((size_t)nb_processus_max * sizeof(task_cell));
    if (tasks == NULL) {
        die("malloc() for tasks failed");
    }

    /* Split [0, TASK_RANGE_MAX] into nb_processus_max contiguous chunks;
       the last chunk absorbs whatever remainder the division leaves. */
    int chunk = TASK_RANGE_MAX / nb_processus_max;
    for (int i = 0; i < nb_processus_max; ++i) {
        tasks[i] = (task_cell){
            .global_sum = &global_sum,
            .mutex = &state_mutex,
            .process_rank = i,
            .partial_sum = 0,
            .socket_process = -1,
            .begin_index = i * chunk,
            .end_index = (i == nb_processus_max - 1) ? (TASK_RANGE_MAX + 1) : (i + 1) * chunk,
            .status = 0,
            .elapsed_time = 0,
        };
    }

    pthread_t manager_thread, report_thread;

    process_manager_param manager_param = {
        .global_sum = &global_sum,
        .mutex = &state_mutex,
        .nb_processus_max = nb_processus_max,
        .sockfd = sockfd,
        .tasks = tasks,
    };
    if (pthread_create(&manager_thread, NULL, process_manager, &manager_param) != 0) {
        fprintf(stderr, "pthread_create error for manager thread\n");
    }

    report_system_param report_param = {
        .global_sum = &global_sum,
        .mutex = &state_mutex,
        .nb_processus_max = nb_processus_max,
        .tasks = tasks,
    };
    if (pthread_create(&report_thread, NULL, report_system, &report_param) != 0) {
        fprintf(stderr, "pthread_create error for report thread\n");
    }

    /* Wait for every task to be marked done before tearing anything down. */
    pthread_join(manager_thread, NULL);

    close(sockfd);

    /* report_system() only ever locks the mutex around a sleep(3), which
       is a cancellation point, so cancelling it here can never leave the
       mutex held. */
    pthread_cancel(report_thread);
    pthread_join(report_thread, NULL);

    pthread_mutex_lock(&state_mutex);
    print_report(nb_processus_max, global_sum, tasks);
    pthread_mutex_unlock(&state_mutex);

    free(tasks);
    pthread_mutex_destroy(&state_mutex);

    printf("Calcul termine\n");

    *flag = 0;
    pthread_exit(NULL);
}
