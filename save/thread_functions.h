/*!
\file thread_functions.h
\version v1
\date Vendredi 19 mars 2020
*/

/*
The declarations of functions for the different thread routines
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

#endif

void* CreateTCPClientSocket(void* arg);
void* monitor(void* arg);
void* evil_monkey(void* arg);
void* process_manager(void* arg);
void* report_system(void* arg);
void* thread_i_function(void *arg);