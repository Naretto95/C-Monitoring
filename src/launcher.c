/*!
\file launcher.c
\brief Entry point: starts the monitor and evil-monkey threads, then
       launches one calculator thread per Enter keypress until the
       monitor reports the computation is complete.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "structures.h"
#include "monitor.h"
#include "thread_functions.h"
#include "calculator.h"

int main(void) {
    pthread_t monitor_thread, evil_monkey_thread;
    volatile int monitor_flag = 1;

    /* Fixed-size and stack-owned for main()'s whole lifetime: nothing
       here needs to be freed, so nothing can be freed too early out from
       under a thread that is still reading it (the original code freed
       this state right before cancelling the threads using it). */
    pthread_t calculators[MAX_CALCULATORS];
    pthread_t calculators_report[MAX_CALCULATORS];
    pthread_t calculators_calcul[MAX_CALCULATORS];
    calculator_param calculator_params[MAX_CALCULATORS];
    int nb_calculators = 0;
    pthread_mutex_t calculators_mutex = PTHREAD_MUTEX_INITIALIZER;

    sem_t monitor_ready;
    sem_init(&monitor_ready, 0, 0);

    srand((unsigned int)time(NULL));

    monitor_init_param monitor_init = {
        .flag = &monitor_flag,
        .ready = &monitor_ready,
    };
    if (pthread_create(&monitor_thread, NULL, monitor, &monitor_init) != 0) {
        fprintf(stderr, "pthread_create error for monitor thread\n");
        return 1;
    }

    evil_monkey_param evil_params = {
        .nb_launched_calculators = &nb_calculators,
        .calculators_mutex = &calculators_mutex,
        .calculators = calculators_report,
        .kill_frequency = DEFAULT_KILL_FREQUENCY,
        .monitor_status = &monitor_flag,
    };
    if (pthread_create(&evil_monkey_thread, NULL, evil_monkey, &evil_params) != 0) {
        fprintf(stderr, "pthread_create error for evil monkey thread\n");
    }

    /* Wait until the monitor is actually listening instead of guessing a
       fixed delay: on a slow/loaded machine a blind sleep() could let a
       calculator try to connect before listen() has run. */
    sem_wait(&monitor_ready);

    /* launch a calculator each time we press Enter */
    while (monitor_flag == 1) {
        if (getchar() == EOF) {
            break;
        }

        pthread_mutex_lock(&calculators_mutex);
        int idx = nb_calculators;
        pthread_mutex_unlock(&calculators_mutex);

        if (idx >= MAX_CALCULATORS) {
            fprintf(stderr, "Nombre maximal de calculateurs atteint (%d)\n", MAX_CALCULATORS);
            break;
        }

        calculator_params[idx].calcul_thread = &calculators_calcul[idx];
        calculator_params[idx].report_thread = &calculators_report[idx];
        if (pthread_create(&calculators[idx], NULL, CreateTCPClientSocket, &calculator_params[idx]) != 0) {
            fprintf(stderr, "pthread_create error for calculator thread\n");
            continue;
        }

        pthread_mutex_lock(&calculators_mutex);
        nb_calculators++;
        pthread_mutex_unlock(&calculators_mutex);

        sleep(1);
    }

    /* grace period so calculators started just before the loop exited
       have time to spin up their sub-threads before we cancel them */
    sleep(2);

    pthread_cancel(monitor_thread);
    pthread_cancel(evil_monkey_thread);
    for (int i = 0; i < nb_calculators; ++i) {
        pthread_cancel(calculators[i]);
        pthread_cancel(calculators_calcul[i]);
        pthread_cancel(calculators_report[i]);
    }

    pthread_mutex_destroy(&calculators_mutex);
    sem_destroy(&monitor_ready);

    printf("Good bye !! :)\n");
    return 0;
}
