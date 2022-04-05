#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "webserver.h"

#define MAX_REQUEST 100

int port, numThread;
int count = 0;
int request[MAX_REQUEST]; // shared buffer

// semaphores and a mutex lock
sem_t sem_full;
sem_t sem_empty;
pthread_mutex_t mutex;


void* begin(void * args){
	int fd;
	while(1){
		sem_wait(&sem_full);
		pthread_mutex_lock(&mutex);
		printf("Worker thread listening!!");
		fd = request[--count];
		pthread_mutex_unlock(&mutex);
		sem_post(&sem_empty);
		process(fd);
	}
	
}

void* req_handler()
{
		int r;
		struct sockaddr_in sin;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);
		int sock;

		sock = socket(AF_INET, SOCK_STREAM, 0);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
		if(r < 0) {
				perror("Error binding socket:");
				exit(0);
		}

		r = listen(sock, 5);
		if(r < 0) {
				perror("Error listening socket:");
				exit(0);
		}

		printf("HTTP server listening on port %d\n", port);

		while (1)
		{
				int s;
				s = accept(sock, NULL, NULL);
				if (s < 0) break;
				pthread_mutex_lock(&mutex);
				request[count++] = s;				
				pthread_mutex_unlock(&mutex);
				sem_post(&sem_full);
		}

		close(sock);
}


int main(int argc, char *argv[])
{
		if(argc < 2 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
		{
				fprintf(stderr, "./webserver PORT(2001 ~ 49999) (#_of_threads) (crash_rate(%))\n");
				return 0;
		}
		
		// port number
		port = atoi(argv[1]);
		
		// # of worker thread
		if(argc > 2) 
				numThread = atoi(argv[2]);
		else numThread = 1;

		// crash rate
		if(argc > 3) 
				CRASH = atoi(argv[3]);
		if(CRASH > 50) CRASH = 50;
		
  sem_init(&sem_empty, 0, MAX_REQUEST);
  sem_init(&sem_full, 0, 0);

		printf("[pid %d] CRASH RATE = %d\%\n", getpid(), CRASH);

		//creating the listener thread
		pthread_t listener;
		if (pthread_create(&listener,NULL,&req_handler,NULL) != 0){
				perror("Creating listener thread failed!!");
			}

		//creating the worker threads
		pthread_t worker[numThread];
		int i;
		for (i = 0 ; i < numThread ; i++){
			if (pthread_create(&worker[i],NULL,&begin,NULL) != 0){
				perror("Creating worker threads failed!!");
			}
		}
		//joining the worker threads
		for (i = 0 ; i < numThread ; i++){
			void *ret;
			pthread_join(worker[i], &ret);
			//handle crash
			while (ret == 0){
				pthread_create(&worker[i],NULL,&begin,NULL);
				pthread_join(worker[i], &ret);
			}
		}
		
		pthread_join(listener, NULL);
		sem_destroy(&sem_empty);
		sem_destroy(&sem_full);
		return 0;
}