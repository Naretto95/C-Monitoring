
#ifndef DEF_FICHIER_H
#define DEF_FICHIER_H

#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for sleep() */
#include <time.h>
#include <pthread.h>    /* for pthread_create() */
#include <stdlib.h>     /* for malloc() */
#include <stdio.h>      /* for perror() */
#include <string.h>
#include "structures.h"
#include "thread_functions.h"

#endif

void DieWithError(char *errorMessage);  /* Error handling function */

typedef struct Buffer
{
	int* msg;
	int sock;
} Buffer;

/* stop the function and send an error*/
void DieWithError(char *errorMessage) {
  perror(errorMessage);
  exit(1);
}

/*function that calculate the partial and increase every 1 sec */
void* partialSum(void* buffer) {
  int* msg = (int*) buffer;
  for (int i = msg[1]; i < msg[2]; i++) {
    msg[0] += i;
    msg[1] = i+1;
    if (sleep(1) != 0)
      DieWithError("sleep() of Sum failed");
  }

  pthread_exit(0);
}

/* funciton that send the partial sum to the monitor every 2 sec */
void* sendSum(void* buffer) {
  Buffer buff = *((Buffer*) buffer);
  int* tab = malloc(2*sizeof(int)+sizeof(time_t));
  time_t start_time;
  time(&start_time);
  while (buff.msg[1] != buff.msg[2]) {

    /* partial sum */
    tab[0] = buff.msg[0];

    /* currentindex */
    tab[1] = buff.msg[1];

    /* timer value update */
    tab[2]=time(NULL) - start_time;

    /* send to server array tab that contains the partial sum and the current index */
    send(buff.sock, tab, 2*sizeof(int)+sizeof(time_t), 0);

    if (sleep(2) != 0)
      DieWithError("sleep() of send() failed");
  }
  /* partial sum */
  tab[0] = buff.msg[0];

  /* currentindex */
  tab[1] = buff.msg[1];

  /* send to server array tab that contains the partial sum and the current index */
  send(buff.sock, tab, 2*sizeof(int), 0);

  free(tab);
  pthread_exit(0);
}

void* CreateTCPClientSocket(void* arg)
{
    int sock;                        /* socket to create */
    struct sockaddr_in echoServAddr; /* Local address */
    calculator_param* params = (calculator_param*)arg; /* we get memory allocation for the calcultor threads */
    int* flag=params->flag;

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* Any incoming interface */
    echoServAddr.sin_port = htons(4206);              /* Local port */

    /* Connect to the server address */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connection to server failed");

    /* Create the buffer that will receive data from the monitor */
    int* msg = malloc(3*sizeof(int));

    /* Mark the socket so it will receive incoming buffers */
    recv(sock, msg, 3*sizeof(int), 0);

    /* Create a thread that calculate the partial sum */
    pthread_create(params->calcul_thread, NULL, partialSum, msg);

    /* Initialize the buffer with the data that will be sent */
    Buffer buffer;
    buffer.msg = msg;
    buffer.sock = sock;

    /* Create a thread that send the partial sum and the current index to the monitor */
    pthread_create(params->report_thread, NULL, sendSum, &buffer);

    pthread_join(*params->calcul_thread,NULL);
    pthread_join(*params->report_thread,NULL);

    close(sock);

    free(msg);

    *flag = 0;

    pthread_exit(0);
}

/*
int main(int argc, char const *argv[]) {
  int flag=1;
  CreateTCPClientSocket(&flag);
  return 0;
}
*/