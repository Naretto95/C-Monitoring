/*!
\file thread_functions.h
\brief Worker-thread routines used by the monitor.
*/

#ifndef THREAD_FUNCTIONS_H
#define THREAD_FUNCTIONS_H

/*! Periodically kills a random calculator's report thread to simulate
    failures. arg is an evil_monkey_param*. */
void *evil_monkey(void *arg);

/*! Accepts incoming calculator connections and assigns them to free task
    slots until every task is done. arg is a process_manager_param*. */
void *process_manager(void *arg);

/*! Prints a periodic snapshot of the monitor's state. arg is a
    report_system_param*. */
void *report_system(void *arg);

/*! Owns the exchange with a single calculator for the lifetime of one
    task assignment. arg is a task_cell*. */
void *thread_i_function(void *arg);

#endif /* THREAD_FUNCTIONS_H */
