/*!
\file structures.h
\brief All shared data structures and wire-format messages used by the
       monitor, the calculators and the launcher.
*/

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/* --- Wire messages -------------------------------------------------- */
/* Calculators run as threads of this same process (connected over
   loopback TCP purely as a simulated network), so both ends share the
   exact same struct layout and sending the struct itself is safe. */

/* Monitor -> calculator: which range to work on, and where to resume
   from if this task is being reassigned after a previous failure. */
typedef struct task_assignment {
    int partial_sum;
    int begin_index;
    int end_index;
} task_assignment;

/* Calculator -> monitor: periodic progress update. */
typedef struct progress_report {
    int partial_sum;
    int next_index;
    time_t elapsed_time;
} progress_report;

/* --- Monitor-side state ---------------------------------------------- */

/* One entry per sub-task of the global sum. Shared between the process
   manager, the per-connection worker threads and the report thread, so
   every access to a field of this struct must hold *mutex. */
typedef struct task_cell {
    int *global_sum;        /* pointer to the monitor's running total */
    pthread_mutex_t *mutex; /* guards global_sum and every field below */
    int process_rank;       /* index of this task, in [0, nb_processus_max) */
    int partial_sum;        /* partial sum reported so far for this task */
    int socket_process;     /* socket used to talk to the calculator currently working on this task, -1 if none */
    int begin_index;        /* next integer to add to the partial sum */
    int end_index;          /* exclusive upper bound of this task's range */
    int status;              /* 0 = unassigned, 1 = in progress, 2 = done */
    time_t elapsed_time;     /* time (s) the current calculator has spent on this task */
} task_cell;

typedef struct report_system_param {
    int *global_sum;
    pthread_mutex_t *mutex;
    int nb_processus_max;
    task_cell *tasks;
} report_system_param;

typedef struct process_manager_param {
    int *global_sum;
    pthread_mutex_t *mutex;
    int nb_processus_max;
    int sockfd;
    task_cell *tasks;
} process_manager_param;

/* Lets monitor() signal the launcher once it is actually listening, so
   the launcher never has to guess how long startup takes. */
typedef struct monitor_init_param {
    volatile int *flag;
    sem_t *ready;
} monitor_init_param;

typedef struct evil_monkey_param {
    int *nb_launched_calculators;
    pthread_mutex_t *calculators_mutex; /* guards *nb_launched_calculators and calculators[] */
    pthread_t *calculators;
    int kill_frequency;
    volatile int *monitor_status;
} evil_monkey_param;

/* --- Calculator-side state --------------------------------------------- */

typedef struct calculator_param {
    pthread_t *report_thread;
    pthread_t *calcul_thread;
} calculator_param;

#endif /* STRUCTURES_H */
