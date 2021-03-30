/*!
\file monitor.c
\version v1
\date Vendredi 19 mars 2020
*/

/*
The monitor of the project
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

#define N 1000

/*
int main(int argc, char const *argv[])
{
	int flag = 1;
	monitor(&flag);
	return 0;
}
*/
void* monitor(void* arg)
{
	int nb_processus_max = 0;
    struct sockaddr_in echoServAddr; 	  			/* Local address */
    unsigned short echoServPort=4206;     			/* Server port */
	int sockfd;										/* the socket of the monitor */
	int global_sum = 0 ;
	tab_cell* tab;									/* we will share the main task between sub tasks, each element of tab contains information about a sub-task progress */
	int* flag = (int*)arg;

	/*let user chose the number of processus to use*/
	printf("Veuillez indiquer le nombre de processus que vous souhaitez utiliser : \n");
	if (scanf("%u",&nb_processus_max)==0){
		perror("Problème avec la saisie du nombre de processus");
	};

	/* Create socket for incoming connections */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket() failed");
    	exit(1);
	}

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sockfd, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
        perror("bind() failed");
    	exit(1);
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sockfd, nb_processus_max) < 0){
        perror("listen() failed");
        exit(1);
    }

	/* initialise the tab */
	tab = malloc(nb_processus_max*sizeof(tab_cell));

	/* we share the work between all the processus */
	for (int i = 0; i < nb_processus_max-1; ++i)
	{
		/* default process socket for each case is nb_processus_max */
		tab_cell cell = {&global_sum,i,0,nb_processus_max,i*(N/(nb_processus_max)),(i+1)*(N/(nb_processus_max)),0,0};
		*(tab+i) = cell;
	}
	tab_cell cell = {&global_sum,nb_processus_max-1,0,nb_processus_max,(nb_processus_max-1)*(N/(nb_processus_max)),N+1,0,0};
	*(tab+nb_processus_max-1) = cell;


	/* we create a thread to manage the different processus */
	pthread_t manager_thread;
	process_manager_param param = {&global_sum,nb_processus_max,sockfd,tab};
	if (pthread_create(&manager_thread, NULL, process_manager,&param)<0){
		fprintf(stderr, "pthread_create error for manager thread\n");
	}

	/* we create an other to show a regular report about the system */
	pthread_t report_thread;
	report_system_param param2 = {&global_sum,nb_processus_max,tab};
	if (pthread_create(&report_thread, NULL, report_system, &param2)<0){
		fprintf(stderr, "pthread_create error for report thread\n");
	}

	/* we wait for the thread to end before to close the monitor */
	pthread_join(manager_thread,NULL);

	/* close the socket */
	close(sockfd);

	/* we don't need the report thread anymore */
	pthread_cancel(report_thread);
	printf("Nombre de processus calculateurs : %d\n", nb_processus_max); /* number of processù*/
	printf("Somme totale partielle calculée : %d\n", global_sum); /*global sum calculated*/
	for (int i = 0; i < nb_processus_max; i++) /* for each process, print sum and time*/
	{
		printf("--PROCESSUS %d\n",i);
		printf("--somme partielle calculée : %d \n",tab[i].partial_sum);
		printf("--Temps : %ld \n",tab[i].time);
		printf("--Etat : %d \n",tab[i].status);
	}

	/*free the memory*/
	free(tab);

	printf("Calcul completed\n");

	*flag = 0;
	pthread_exit(0);
}