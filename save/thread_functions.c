/*!
\file thread_functions.c
\version v1
\date Vendredi 19 mars 2020
*/

/*
The implementation of the functions for the different thread routines
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
#include "structures.h"
#include "thread_functions.h"

#endif

void* evil_monkey(void* arg){
    evil_monkey_param* params = (evil_monkey_param*) arg;
    int has_killed=1;
    int calculators_size;
    int nb_killed=0;
    /* until monitor is finished */
    while (*(params->monitor_status)!=0)
    {
        sleep(params->kill_frequency);
    	while(has_killed!=0){
    		/* evil monkey is looking for a victim */
    		calculators_size = *(params->nb_launched_calculators);
    		printf("--------------------------------------Evil Monkey--looking for a victim among %d processes\n", calculators_size);
    		/* try to kill the report_thread from a calculator */
    		if (calculators_size>0){has_killed=pthread_cancel(params->calculators[rand()%(calculators_size)]);}
    		sleep(1);
    	}
    	nb_killed++;
    	printf("----------------------------------------Evil Monkey--I've killed %d since monitor started\n",nb_killed);
    	has_killed = 1;
    	/* it repeat the operation each kill_frequency seconds */
    }
    pthread_exit(0);
}


void* process_manager(void* arg){

	process_manager_param param = *((process_manager_param*) arg); /* we get the parameters */
	int nb_processus_max = param.nb_processus_max;					
	int sockfd = param.sockfd;
	int vacancy=400;	/* vacancy variable contains the index of a cell with no processus working on, it's equal to 400 if all cells are working */
	int i = 0;
	while(1){
		sleep(1);
		/* if cell has no processus working on it */
		if (vacancy!=400){

			struct sockaddr_in echoClntAddr;
			unsigned int clntLen = sizeof(echoClntAddr);
			int new_client;

			/*we accept a new calculator-processus for the vacancy cell */
			if ((new_client = accept(sockfd, (struct sockaddr *) &echoClntAddr, &clntLen))<0){
				perror("accept() failed");
			}else{

				pthread_t thread;
				(param.tab+vacancy)->socket_process = new_client;
				(param.tab+vacancy)->status = 1;

				/* we create a thread to exchange with this new processus */
				if (pthread_create(&thread, NULL, thread_i_function,param.tab+vacancy)<0){
					fprintf(stderr, "pthread_create error for thread\n");
					(param.tab+vacancy)->status = 0;
				}

				/* then the cell is not free anymore so vacancy is set to 400 */
				vacancy = 400;
			}
		}
		i = 0;
		/* now we look for an other free call */
		for (int j = 0; j < nb_processus_max; ++j)
		{
			if((param.tab+j)->status==2){i++;}
			if((param.tab+j)->status==0){vacancy = j;break;}
		}
		if(i==nb_processus_max){
			break;
		}
	}
	//printf("Calcul completed\n");
	pthread_exit(0);
};

void* report_system(void* arg){
	report_system_param param = *((report_system_param*) arg);
	while(1){
		printf("Nombre de processus calculateurs : %d\n", param.nb_processus_max); /* number of processù*/
		printf("Somme totale partielle calculée : %d\n", *(param.global_sum)); /*global sum calculated*/
		for (int i = 0; i < param.nb_processus_max; i++) /* for each process, print sum and time*/
		{
			printf("--PROCESSUS %d\n",i);
			printf("--somme partielle calculée : %d \n",param.tab[i].partial_sum);
			printf("--Temps : %ld \n",param.tab[i].time);
			printf("--Etat : %d \n",param.tab[i].status);
		}
		sleep(3);
	}
	pthread_exit(0);
};

void * send_info(tab_cell * params){

	int message[3] = {params->partial_sum,params->begin_index,params->end_index};
    send(params->socket_process,message,3*sizeof(int),0);
    return NULL;
}


void* thread_i_function(void *arg){
    int acc = 0;
    struct tab_cell * params= ((tab_cell*)arg);
    send_info(params);
    int* message = malloc(2*sizeof(int)+sizeof(time_t));
    int delta;

    while ((params->begin_index < params->end_index) && (acc<6)) {
        if (recv(params->socket_process,message,2*sizeof(int)+sizeof(time_t),MSG_DONTWAIT) <= 0 ){
            acc++;
        }else{
        	acc=0;
        	delta = message[0] - params->partial_sum;
            params->begin_index = message[1];
            params->partial_sum = message[0];
            params->time = message[2];
            *(params->global_sum)+=delta;
        }
        sleep(1);
    }
	if (params->begin_index < params->end_index){
		params->status = 0;
	}else{
		params->status = 2;
	}
    pthread_exit(0);
};