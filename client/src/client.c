#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kos_net.h>

#define STATE_REQUEST (-1)
#define EXAMPLE_HOST "127.0.0.1"
//#define EXAMPLE_HOST "localhost" //cannot get address info =(: File exists
#define EXAMPLE_PORT 7777
#define NUM_RETRIES 10

static const char EntityName[] = "Client";

typedef struct {
    int dir0;
    int dir1;
} direction;

static void print_directions(direction* dir){
    fprintf(stderr, "[%s] Current state is dir0 = 0x%02x; dir1 = 0x%02x\n", EntityName, (int) (dir->dir0  & 0xFF), (int) (dir->dir1 & 0xFF));
}

direction prev_dir = {-1, -1};


static int poll_direction(direction* dir){
    /* Create socket for connection with server. */
    int clientSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == clientSocketFD){
        perror("cannot create socket");
        return EXIT_FAILURE;
    }

    /* Creating and initialisation of socket`s address structure for connection with server. */
    struct sockaddr_in stSockAddr;
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(EXAMPLE_PORT);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *resultHints = NULL;
    int res = getaddrinfo(EXAMPLE_HOST, NULL, &hints, &resultHints);
    if (res != 0){
        perror("cannot get address info =(");
        close(clientSocketFD);
        return EXIT_FAILURE;
    }

    struct sockaddr_in* in_addr = (struct sockaddr_in *)resultHints->ai_addr;
    memcpy(&stSockAddr.sin_addr.s_addr, &in_addr->sin_addr.s_addr, sizeof(in_addr_t));
    freeaddrinfo(resultHints);

    res = -1;
    for (int i = 0; res == -1 && i < NUM_RETRIES; i++){
        sleep(1); // Wait some time for server be ready.
        res = connect(clientSocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
    }

    if (res == -1){
        perror("connect failed");
        close(clientSocketFD);
        return EXIT_FAILURE;
    }

    int state = STATE_REQUEST;
    /* Send data to server. */
    if (-1 == send(clientSocketFD, &state, sizeof(state),0)) {
        perror("send failed");
        close(clientSocketFD);
        return EXIT_FAILURE;
    }

    /* Get data from server. */
    if (-1 == recv(clientSocketFD, dir, sizeof(*dir),0)){
        perror("recv failed");
        close(clientSocketFD);
        return EXIT_FAILURE;
    }

    /* Close connection with server. */
    shutdown(clientSocketFD, SHUT_RDWR);
    close(clientSocketFD);

    return EXIT_SUCCESS;
}



int main(void){

    fprintf(stderr, "Hello I'm %s\n", EntityName);

    /* Initialisation of network interface "en0". */
    if (!configure_net_iface(DEFAULT_INTERFACE, DEFAULT_ADDR, DEFAULT_MASK,
                             DEFAULT_GATEWAY, DEFAULT_MTU)){
        perror("can not init network");
        return EXIT_FAILURE;
    }

    while (1){

        direction dir;
        if(EXIT_SUCCESS != poll_direction(&dir)){
            continue;
        }

        if(prev_dir.dir0 == dir.dir0 &&
           prev_dir.dir1 == dir.dir1){
            continue;
        }

        prev_dir = dir;
        print_directions(&prev_dir);
    }

    return EXIT_SUCCESS;
}