#ifndef TRAFFIC_LIGHT_SERVER_H
#define TRAFFIC_LIGHT_SERVER_H

typedef struct {
    int dir0;
    int dir1;
} direction;

void server_run(int port);
void server_send(direction dir);
void server_stop(void);

#endif //TRAFFIC_LIGHT_SERVER_H
