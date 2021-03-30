#ifndef DEF_FICHIER_H
#define DEF_FICHIER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include "structures.h"
#include "thread_functions.h"
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <time.h>
#include <string.h>

#endif

int main()
{
    pthread_t monitor_thread,evil_monkey_thread;
    int* monitor_flag=malloc(sizeof(int));
    *monitor_flag=1;

    /* we allocate memory for the calculators because we need to keep a reference on them for the evil monkey */
    pthread_t* calculators = malloc(200*sizeof(pthread_t));
    pthread_t* calculators_report = malloc(200*sizeof(pthread_t));
    pthread_t* calculators_calcul = malloc(200*sizeof(pthread_t));
    int* nb_calculators=malloc(sizeof(int));
    *nb_calculators=0;
    int evil_monkey_kill_frequency = 20;
    evil_monkey_param params = {nb_calculators,calculators_report,evil_monkey_kill_frequency,monitor_flag};

    /*launch monitor with flag 1*/
    pthread_create(&monitor_thread,NULL,monitor,monitor_flag);

    /*launch evil_monkey */
    pthread_create(&evil_monkey_thread,NULL,evil_monkey,&params);

    sleep(2);
    /* launch a calculator each we press Enter */
    while(*monitor_flag==1){
        getchar();
        calculator_param param; 
        param.calcul_thread = &calculators_calcul[*nb_calculators];
        param.report_thread = &calculators_report[*nb_calculators];
        pthread_create(&calculators[*nb_calculators],NULL,CreateTCPClientSocket,&param);
        *nb_calculators += 1;
        sleep(1);
    }

    sleep(2);
    /* we free the memory */
    free(monitor_flag);
    free(nb_calculators);
    free(calculators);

    /* we kill the zombie threads after evil monkey attack */
    pthread_cancel(monitor_thread);
    pthread_cancel(evil_monkey_thread);
    for (int i = 0; i < *nb_calculators; ++i)
    {
        pthread_cancel(calculators[i]);
        pthread_cancel(calculators_calcul[i]);
    }
    printf("Good bye !! :)\n");
    return 0;
}


