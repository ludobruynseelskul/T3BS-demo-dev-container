#include "lib/tcpsock.h"
#include "connmngr.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include "config.h"

extern int MAX_CONN ;
extern int PORT ;

int total_counter = 0;
int open_counter = 0;

void *request_runner(void *param);

pthread_mutex_t mutex;
pthread_cond_t spawned;

int start_listening()
{

    pthread_mutex_init(&mutex, NULL);
     pthread_cond_init(&spawned, NULL);

    tcpsock_t *server, *client;

    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR)
        exit(EXIT_FAILURE);

    do //while loop until MAX CONN reached
    {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR)
            exit(EXIT_FAILURE);

        pthread_t tid;
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, request_runner, client);
        printf("Thread spawned: %lu\n", tid);

        pthread_mutex_lock(&mutex); 
        pthread_cond_wait(&spawned, &mutex); //wait till thread is spawned and counted
         printf("Current total connections: %d\n", total_counter);
        if (total_counter == MAX_CONN)
        {
            
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

    } while (1);

    while(1){ //waiting for all requests to end after MAXCONN has been reached

            pthread_mutex_lock(&mutex);
            if(open_counter == 0){
                    printf("Current open connections: %d\n", open_counter);
                   pthread_mutex_unlock(&mutex);
                   break;

            }
           pthread_mutex_unlock(&mutex);

            sleep(10);
           

    }

    if (tcp_close(&server) != TCP_NO_ERROR)
        exit(EXIT_FAILURE);

    return 0;
}

int stop_listening()
{
}

void *request_runner(void *param)
{
    tcpsock_t *client = (tcpsock_t *)param;
    sensor_data_t data;
    int bytes, result;

    printf("Incoming client connection\n");

// critical section for total connections and open connections. signal listener at end when counts are up to date
    pthread_mutex_lock(&mutex);
    total_counter++;
    open_counter++;
    pthread_cond_signal(&spawned); //notify listener that I am spawned and counted for
    pthread_mutex_unlock(&mutex);

    do
    {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *)&data.id, &bytes);
        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *)&data.value, &bytes);
        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *)&data.ts, &bytes);
        if ((result == TCP_NO_ERROR) && bytes)
        {
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value,
                   (long int)data.ts);
        }
    } while (result == TCP_NO_ERROR);
    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occured on connection to peer\n");
    tcp_close(&client);

    pthread_mutex_lock(&mutex);
    open_counter--;
    pthread_mutex_unlock(&mutex);

    pthread_exit(0);
}