/**
 * \author {AUTHOR}
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "pthread.h"


//#define NDEBUG
#include "dbg.h"

int PORT = 0;
int MAX_CONN = 0;
pthread_barrier_t start_barrier;

void *thread_function(void *arg);

/**
 * Implements a sequential test server (only one connection at the same time)
 */

int main(int argc, char *argv[]) {
    tcpsock_t *server;
    int conn_counter = 0;
    pthread_t *thread_list;
    tcpsock_t **socket_list;

    if (argc < 3) {
        log_err("Please provide the right arguments: first the port, then the max nb of clients");
        return -1;
    }

    MAX_CONN = atoi(argv[2]);
    PORT = atoi(argv[1]);
    thread_list = malloc(sizeof(pthread_t) * MAX_CONN);
    socket_list= malloc(sizeof(tcpsock_t*) * MAX_CONN);

    log_info("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    do {
        pthread_t *next_thread = &thread_list[conn_counter];
        tcpsock_t *next_socket = socket_list[conn_counter];
        conn_counter++;
        debug("Connection counter = %d", conn_counter);

        if (tcp_wait_for_connection(server, &next_socket) != TCP_NO_ERROR) 
            exit(EXIT_FAILURE);

        log_info("Incoming client connection\n");
        
        pthread_barrier_init(&start_barrier, NULL, 2);
        pthread_create(next_thread, NULL, thread_function, next_socket);


        pthread_t thr = *next_thread;

        char buffer[16];
        sprintf(buffer, "conn-%d", conn_counter);
        int res = pthread_setname_np(thr, buffer);
        if (res != 0) 
            log_err("error setting thread name. erno  %d", res);
        else
            log_info("Thread name set to %s", buffer);

        pthread_barrier_wait(&start_barrier);
        //sleep(1);
        char  thname[16];

        res = pthread_getname_np( pthread_self(), thname, 16);
        if (res != 0) 
            log_err("error getting thread name. erno  %d", res);
        else
            log_info("Thread name is %s", buffer);

        log_info("Current number of connections: %i\n",conn_counter);
    } while (conn_counter < MAX_CONN);
    
    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    for(int i = 0; i<MAX_CONN; i++){
        pthread_join(thread_list[i], NULL);
    }
    log_info("Test server is shutting down\n");
    free(socket_list);
    free(thread_list);
    return 0;
}

void *thread_function(void *arg) {
        pthread_barrier_wait(&start_barrier);
    int bytes, result;
    sensor_data_t data;
    tcpsock_t *client = (tcpsock_t *) arg;
    char  thname[16];
    pthread_t th = pthread_self();

    int res = pthread_getname_np(th, thname, 16);
    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void *) &data.id, &bytes);
        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void *) &data.value, &bytes);
        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void *) &data.ts, &bytes);

        if ((result == TCP_NO_ERROR) && bytes) {
            log_info("thread = %s - sensor id = %"
            PRIu16
            " - temperature = %g - timestamp = %ld\n",thname, data.id, data.value,
                    (long int) data.ts);
        }
    } while (result == TCP_NO_ERROR);
    if (result == TCP_CONNECTION_CLOSED)
        log_info("Peer has closed connection\n");
    else
        log_info("Error occured on connection to peer\n");
    tcp_close(&client);
}



