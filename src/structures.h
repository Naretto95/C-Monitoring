/*!
\file structures.h
\version v1
\date Vendredi 19 mars 2020
*/

/*!
Here are all the structures we need in our project
*/

#ifndef DEF_FICHIER_H
#define DEF_FICHIER_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h> 
#include <time.h>
#include "thread_functions.h"

#endif

/* each task(sub-calcul) has its data in tab_cell */
typedef struct tab_cell
{ 
	int* global_sum; /* pointer to the global sum of the monitor*/
	int process_rank; /* the rank of the task, [1,100] would be the first task for example */
	int partial_sum; /* partial sum for this task */
	int socket_process; /* the socket to recv information about the calculator working on this task */ 
	int begin_index; /* the index of the next number tu add to the partial sum */
	int end_index;/* the index of the last number for this task */
	int status;/* give information about the task current status, 0:no calculator working on, 1: calculator working on,2: task ended */
	time_t time;/* the last calculator started to work on this task for time seconds */
} tab_cell;


/* parameter of the function which show synthesis of the monitor */
typedef struct report_system_param
{
	int* global_sum;
	int nb_processus_max;
	tab_cell* tab; /* pointer to all tasks */
} report_system_param;

/* parameter of the function which is assigning tasks to the inbound calculators */
typedef struct process_manager_param
{
	int* global_sum;
	int nb_processus_max;
	int sockfd;
	tab_cell* tab;
} process_manager_param;

typedef struct evil_monkey_param
{
	int* nb_launched_calculators;
	pthread_t* calculators;
	int kill_frequency;
	int* monitor_status;
}evil_monkey_param;


typedef struct calculator_param
{
	int* flag;
	pthread_t* report_thread;
	pthread_t* calcul_thread;
}calculator_param;

