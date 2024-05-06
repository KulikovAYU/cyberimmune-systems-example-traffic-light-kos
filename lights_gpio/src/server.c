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
    direction buffer;
    pthread_mutex_t mtx;
} direction_buffer;

pthread_t ctrl_tid;
direction_buffer* cb_ptr;
int serverSocketFD = -1;
static const char EntityName[] = "Server";

static void destroy_buffer(direction_buffer* db){
    pthread_mutex_destroy(&db->mtx);
    free(db);
}

static direction_buffer* create_buffer(void){
    if(cb_ptr != NULL)
        destroy_buffer(cb_ptr);

    direction_buffer* db = (direction_buffer*)malloc(sizeof(direction_buffer));
    if (db == NULL) {
        fprintf(stderr, "Error: Memory allocation for direction_buffer failed.\n");
        return NULL;
    }

    pthread_mutex_init(&db->mtx, NULL);
    fprintf(stderr, "[%s] Buffer was created\n", EntityName);

    return db;
}

static void enqueue(direction_buffer* db, direction data) {
    pthread_mutex_lock(&db->mtx);
    db->buffer = data;
    pthread_mutex_unlock(&db->mtx);
}

static int dequeue(direction_buffer* db, direction* out_direction){
    pthread_mutex_lock(&db->mtx);
    *out_direction = db->buffer;
    pthread_mutex_unlock(&db->mtx);
    return 1;
}

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

    fprintf(stderr, "[%s] is now listening port %d\n", EntityName, port);

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
    cb_ptr = create_buffer();
    pthread_create(&ctrl_tid, NULL, do_start_server, &port);
}

void server_send(direction dir){
    assert(cb_ptr != NULL && "Error! Buffer was not allocated!\n");
    enqueue(cb_ptr, dir);
    fprintf(stderr, "[%s] Sending current state\t dir0 = 0x%02x; dir1 = 0x%02x\n",  EntityName, (int) (dir.dir0  & 0xFF), (int) (dir.dir1 & 0xFF));
}

void server_stop(void){
    /* Stop processing of incoming connection requests. */
    shutdown(serverSocketFD, SHUT_RDWR);
    close(serverSocketFD);
    pthread_join(ctrl_tid, NULL);
    destroy_buffer(cb_ptr);
}