/*!
\file thread_functions.c
\brief Implementation of the monitor's worker-thread routines: task
       assignment, progress reporting, per-calculator bookkeeping and the
       evil monkey failure injector.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"
#include "structures.h"
#include "thread_functions.h"

void *evil_monkey(void *arg) {
    evil_monkey_param *params = (evil_monkey_param *)arg;
    int nb_killed = 0;

    while (*(params->monitor_status) != 0) {
        sleep(params->kill_frequency);

        int killed_one = 0;
        while (!killed_one) {
            pthread_mutex_lock(params->calculators_mutex);
            int calculators_size = *(params->nb_launched_calculators);
            pthread_t victim = 0;
            if (calculators_size > 0) {
                victim = params->calculators[rand() % calculators_size];
            }
            pthread_mutex_unlock(params->calculators_mutex);

            printf("-- Evil Monkey: looking for a victim among %d calculator(s)\n", calculators_size);

            if (calculators_size > 0) {
                killed_one = (pthread_cancel(victim) == 0);
            }
            sleep(1);
        }
        nb_killed++;
        printf("-- Evil Monkey: killed %d calculator(s) since the monitor started\n", nb_killed);
    }
    pthread_exit(NULL);
}

/* Sends a calculator the range it should work on, resuming from wherever
   this task cell last got to (relevant when a task is being reassigned
   after its previous calculator was killed). */
static void send_task_assignment(task_cell *cell) {
    task_assignment assignment;

    pthread_mutex_lock(cell->mutex);
    assignment.partial_sum = cell->partial_sum;
    assignment.begin_index = cell->begin_index;
    assignment.end_index = cell->end_index;
    pthread_mutex_unlock(cell->mutex);

    if (send(cell->socket_process, &assignment, sizeof(assignment), 0) < 0) {
        perror("send() of task assignment failed");
    }
}

void *process_manager(void *arg) {
    process_manager_param param = *((process_manager_param *)arg);
    int nb_processus_max = param.nb_processus_max;
    int sockfd = param.sockfd;
    int vacancy = NO_VACANCY;

    while (1) {
        sleep(1);

        if (vacancy != NO_VACANCY) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

            if (new_client < 0) {
                perror("accept() failed");
            } else {
                task_cell *cell = param.tasks + vacancy;

                pthread_mutex_lock(param.mutex);
                cell->socket_process = new_client;
                cell->status = 1;
                pthread_mutex_unlock(param.mutex);

                pthread_t worker;
                if (pthread_create(&worker, NULL, thread_i_function, cell) != 0) {
                    fprintf(stderr, "pthread_create error for worker thread\n");
                    pthread_mutex_lock(param.mutex);
                    cell->status = 0;
                    pthread_mutex_unlock(param.mutex);
                    close(new_client);
                } else {
                    pthread_detach(worker);
                }
                vacancy = NO_VACANCY;
            }
        }

        int nb_done = 0;
        pthread_mutex_lock(param.mutex);
        for (int j = 0; j < nb_processus_max; ++j) {
            if (param.tasks[j].status == 2) {
                nb_done++;
            } else if (param.tasks[j].status == 0 && vacancy == NO_VACANCY) {
                vacancy = j;
            }
        }
        pthread_mutex_unlock(param.mutex);

        if (nb_done == nb_processus_max) {
            break;
        }
    }
    pthread_exit(NULL);
}

void *report_system(void *arg) {
    report_system_param param = *((report_system_param *)arg);

    while (1) {
        pthread_mutex_lock(param.mutex);
        printf("Nombre de processus calculateurs : %d\n", param.nb_processus_max);
        printf("Somme totale partielle calculee : %d\n", *(param.global_sum));
        for (int i = 0; i < param.nb_processus_max; i++) {
            printf("--PROCESSUS %d\n", i);
            printf("--somme partielle calculee : %d \n", param.tasks[i].partial_sum);
            printf("--Temps : %ld \n", (long)param.tasks[i].elapsed_time);
            printf("--Etat : %d \n", param.tasks[i].status);
        }
        pthread_mutex_unlock(param.mutex);
        /* Unlocked before sleep() (a cancellation point), so the monitor
           can safely pthread_cancel() this thread once it is done with
           it without ever catching the mutex held. */
        sleep(3);
    }
    pthread_exit(NULL);
}

/* I/O with a calculator is inherently un-synchronized message framing:
   each progress_report is sent in a single send() and polled for in a
   single recv(), which is safe here only because messages are small,
   spaced roughly 2s apart, and delivered over loopback. */
void *thread_i_function(void *arg) {
    task_cell *cell = (task_cell *)arg;
    int consecutive_misses = 0;
    int begin, end;

    send_task_assignment(cell);

    pthread_mutex_lock(cell->mutex);
    begin = cell->begin_index;
    end = cell->end_index;
    pthread_mutex_unlock(cell->mutex);

    while (begin < end && consecutive_misses < TASK_MAX_MISSED_REPORTS) {
        progress_report report;
        ssize_t n = recv(cell->socket_process, &report, sizeof(report), MSG_DONTWAIT);

        if (n != (ssize_t)sizeof(report)) {
            consecutive_misses++;
        } else {
            consecutive_misses = 0;
            int delta = report.partial_sum - cell->partial_sum;

            pthread_mutex_lock(cell->mutex);
            cell->begin_index = report.next_index;
            cell->partial_sum = report.partial_sum;
            cell->elapsed_time = report.elapsed_time;
            *(cell->global_sum) += delta;
            pthread_mutex_unlock(cell->mutex);

            begin = report.next_index;
        }
        sleep(1);
    }

    close(cell->socket_process);

    pthread_mutex_lock(cell->mutex);
    cell->socket_process = -1;
    cell->status = (cell->begin_index < cell->end_index) ? 0 : 2;
    pthread_mutex_unlock(cell->mutex);

    pthread_exit(NULL);
}
