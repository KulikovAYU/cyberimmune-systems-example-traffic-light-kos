#include "server.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define MAX_PENDING_CONNECTIONS 10
#define STATE_REQUEST -1
#define STATE_UNKNOWN -2

typedef struct{
    direction *buffer;
    int head;
    int tail;
    int size;
    pthread_mutex_t mtx;
} circular_buffer;

pthread_t ctrl_tid;
circular_buffer* cb_ptr;
int serverSocketFD = -1;

static void destroy_buffer(circular_buffer* cb){
    free(cb->buffer);
    pthread_mutex_destroy(&cb->mtx);
    free(cb);
}

static circular_buffer* create_buffer(int buff_size){
    if(cb_ptr != NULL)
        destroy_buffer(cb_ptr);

    circular_buffer* cb = (circular_buffer*)malloc(sizeof(circular_buffer));
    if (cb == NULL) {
        fprintf(stderr, "Error: Memory allocation for circular_buffer failed.\n");
        return NULL;
    }

    cb->buffer = (direction*) malloc((unsigned int)buff_size * sizeof(direction));
    if (cb->buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation for buffer failed.\n");
        free(cb);
        return NULL;
    }

    cb->head = 0;
    cb->tail = 0;
    cb->size = buff_size;
    pthread_mutex_init(&cb->mtx, NULL);

    fprintf(stderr, "Buffer was created\n");
    return cb;
}

static void enqueue(circular_buffer* cb,direction data) {
    pthread_mutex_lock(&cb->mtx);
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    if (cb->head == cb->tail) {
        cb->tail = (cb->tail + 1) % cb->size;
    }
    pthread_mutex_unlock(&cb->mtx);
}

static int dequeue(circular_buffer* cb, direction* out_direction){
    pthread_mutex_lock(&cb->mtx);

    if (cb->head == cb->tail) {
        //fprintf(stderr, "Buffer is empty\n");
        pthread_mutex_unlock(&cb->mtx);
        return 0;
    }

    *out_direction = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    pthread_mutex_unlock(&cb->mtx);

    return 1;
}
//static direction dequeue(circular_buffer* cb) {
//    pthread_mutex_lock(&cb->mtx);
//    direction empty_direction = {-1, -1};
//    if (cb->head == cb->tail) {
//        fprintf(stderr, "Buffer is empty\n");
//        pthread_mutex_unlock(&cb->mtx);
//        return empty_direction;
//    }
//    direction data = cb->buffer[cb->tail];
//    cb->tail = (cb->tail + 1) % cb->size;
//    pthread_mutex_unlock(&cb->mtx);
//
//    return data;
//}

static void* do_start_server(void *ctx){
    int port = *(int*)ctx;

    /* Create receiving socket for incomming connection requests. */
    serverSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(-1 == serverSocketFD){
        perror("can not create socket");
        return NULL;
    }

    struct sockaddr_in stSockAddr;
    /* Create and initialise socket`s address structure. */
    memset(&stSockAddr, 0, sizeof(stSockAddr));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    stSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind socket with local address. */
    if(-1 == bind(serverSocketFD,(struct sockaddr *)&stSockAddr, sizeof(stSockAddr))){
        perror("error bind failed");
        close(serverSocketFD);
        goto exit;
    }

    /* Set socket to listening state. */
    if(-1 == listen(serverSocketFD, MAX_PENDING_CONNECTIONS)){
        perror("error listen failed");
        close(serverSocketFD);
        goto exit;
    }

    fprintf(stderr, "Server is now listening port %d\n", port);

    fd_set readset, writeset;

    while (1){
        FD_ZERO(&readset);
        FD_SET(serverSocketFD, &readset);

        FD_ZERO(&writeset);
        FD_SET(serverSocketFD, &writeset);

        /* Waiting for incoming connection request. */
        if (select(serverSocketFD + 1, &readset, &writeset, NULL, NULL) == 1){
            int state = STATE_UNKNOWN;
            int ConnectFD = -1;

            if (FD_ISSET(serverSocketFD, &readset)){
                /* Receiving of incoming connection request. */
                ConnectFD = accept(serverSocketFD, NULL, NULL);
                if (-1 == ConnectFD){
                    perror("error accept failed");
                    close(serverSocketFD);
                    goto exit;
                }

                /* Read data from incoming connection. */
                if (-1 == recv(ConnectFD, &state, sizeof(state), 0)){
                    perror("recv failed");
                    close(ConnectFD);
                    close(serverSocketFD);
                    goto exit;
                }
            }

            if (STATE_REQUEST == state){
                direction dir = {-1, -1};
                if(dequeue(cb_ptr, &dir) > 0){
                    //fprintf(stderr, "After request direction is dir0 = 0x%02x; dir1 = 0x%02x\n", (int) (dir.dir0  & 0xFF), (int) (dir.dir1 & 0xFF));
                    if (-1 == send(ConnectFD, &dir, sizeof(dir), 0)){
                        perror("send failed");
                        close(ConnectFD);
                        goto exit;
                    }
                }
            }

            /* Close incomming connection. */
            if (-1 == shutdown(ConnectFD, SHUT_RDWR)){
                perror("can not shutdown socket");
                close(ConnectFD);
                close(serverSocketFD);
                goto exit;
            }

            /* Close accepted connection handler. */
            close(ConnectFD);
        }else{
            /* In case of error close receive socket. */
            close(serverSocketFD);
            return NULL;
        }
    }

    return NULL;

    exit:
        destroy_buffer(cb_ptr);
        return NULL;
}

void server_run(int port){
    //alloc buffer
    cb_ptr = create_buffer(10);
    pthread_create(&ctrl_tid, NULL, do_start_server, &port);
}

void server_send(direction dir){
    assert(cb_ptr != NULL && "Error! Buffer was not allocated!\n");
    enqueue(cb_ptr, dir);
    fprintf(stderr, "Sending current state\t dir0 = 0x%02x; dir1 = 0x%02x\n", (int) (dir.dir0  & 0xFF), (int) (dir.dir1 & 0xFF));
}

void server_stop(void){
    /* Stop processing of incoming connection requests. */
    shutdown(serverSocketFD, SHUT_RDWR);
    close(serverSocketFD);
    pthread_join(ctrl_tid, NULL);
    destroy_buffer(cb_ptr);
}