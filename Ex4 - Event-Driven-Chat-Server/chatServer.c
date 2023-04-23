#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <stdbool.h>

#include <signal.h>

#include <sys/ioctl.h>

#include <sys/select.h>

#include "chatServer.h"

#define BUFFER_SIZE 4096
#define FAILURE (-1)
#define SUCCESS 0
static int end_server = false;
int Main_Socket;
void destroy(conn_pool_t * pool); //free all the allocation that have been created
int Create_server(struct sockaddr_in * my_server, int port); //the function create the server and setup
int StringToInt(char num[]); // this method turn the String to int
int IsDigits(const char * num); //check if the string has only numbers
void intHandler(int SIG_INT); //this function will handle the signal ctrl+C
void intHandler(int SIG_INT) {
    if (SIG_INT == SIGINT) {
        end_server = true;
    }
}
int IsDigits(const char * num) { //check if the string has only numbers
    for (int i = 0; num[i] != '\0'; i++) {
        if (num[i] < '0' || num[i] > '9')
            return FAILURE;
    }
    return SUCCESS;
}
int StringToInt(char num[]) // this method turn the String to int
{
    int size = (int) strlen(num);
    int i, counter = 1, number = 0;
    for (i = size - 1; i >= 0; i--) {
        number += ((num[i] - '0') * counter);
        counter *= 10;
    }
    return number;
}
int Create_server(struct sockaddr_in * my_server, int port) { //the function create the server and setup
    my_server -> sin_family = AF_INET;
    my_server -> sin_addr.s_addr = INADDR_ANY;
    my_server -> sin_port = htons(port);
    Main_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Main_Socket < 0) {
        perror("error: socket\n");
        return FAILURE;
    }
    Main_Socket = Main_Socket;
    if (bind(Main_Socket, (struct sockaddr * ) my_server, sizeof(struct sockaddr_in)) < 0) {
        perror("error: bind\n");
        if (close(Main_Socket) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    int on = 1;
    int rc = ioctl(Main_Socket, (int) FIONBIO, (char * ) & on);
    if (rc < 0) {
        perror("error: ioctl\n");
        if (close(Main_Socket) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    if (listen(Main_Socket, 10) < 0) {
        perror("error: listen\n");
        if (close(Main_Socket) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    return SUCCESS;
}
int main(int argc, char * argv[]) {
    signal(SIGINT, intHandler);
    if (argc != 2 || IsDigits(argv[1])) {
        printf("Usage: chatServer <port>\n");
        exit(EXIT_FAILURE);
    }
    int port = StringToInt(argv[1]);
    conn_pool_t * pool = calloc(1, sizeof(conn_pool_t));
    if (init_pool(pool) < 0) {
        exit(EXIT_FAILURE);
    }
    int FD_client, rc;
    struct sockaddr_in my_server, client_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    if (Create_server( & my_server, port) == FAILURE) {
        free(pool);
        exit(EXIT_FAILURE);
    }
    pool -> maxfd = Main_Socket;
    FD_SET(Main_Socket, & pool -> read_set);
    char buff[BUFFER_SIZE + 1];
    int byte_read;
    do {
        FD_ZERO( & pool -> ready_read_set);
        FD_ZERO( & pool -> ready_write_set);
        pool -> ready_read_set = pool -> read_set;
        pool -> ready_write_set = pool -> write_set;
        printf("Waiting on select()...\nMaxFd %d\n", pool -> maxfd);
        rc = select(pool -> maxfd + 1, & pool -> ready_read_set, & pool -> ready_write_set, NULL, NULL);
        if (rc < 0) {
            if (end_server == false)
                perror("error: select\n");
            break;
        }
        pool -> nready = rc;
        int MAX_Clients = pool -> maxfd;
        for (int FD_Index = 0; FD_Index <= MAX_Clients; FD_Index++) {
            if (pool -> nready == 0) break;
            if (FD_ISSET(FD_Index, & pool -> ready_read_set)) {
                pool -> nready--;
                FD_CLR(FD_Index, & pool -> ready_read_set);
                if (FD_Index == Main_Socket) {
                    FD_client = accept(Main_Socket, (struct sockaddr * ) & client_addr, & socklen);
                    if (FD_client < 0) {
                        continue;
                    }
                    printf("New incoming connection on sd %d\n", FD_client);
                    if (add_conn(FD_client, pool) == FAILURE) {
                        close(FD_client);
                    }
                    continue;
                } else {
                    printf("Descriptor %d is readable\n", FD_Index);
                    memset(buff, '\0', BUFFER_SIZE + 1);
                    if ((byte_read = (int) read(FD_Index, buff, BUFFER_SIZE)) < 0) {
                        continue;
                    }
                    printf("%d bytes received from sd %d\n", byte_read, FD_Index);
                    if (byte_read == 0) {
                        printf("Connection closed for sd %d\n", FD_Index);
                        if (remove_conn(FD_Index, pool) == FAILURE) {
                            continue;
                        }
                        printf("removing connection with sd %d \n", FD_Index);
                        continue;
                    }
                    if (add_msg(FD_Index, buff, byte_read, pool) == FAILURE) {
                        continue;
                    }
                }
            }
            if (FD_ISSET(FD_Index, & pool -> ready_write_set)) {
                pool -> nready--;
                FD_CLR(FD_Index, & pool -> ready_write_set);
                if (write_to_client(FD_Index, pool) == FAILURE) {
                    continue;
                }
            }
        }

    } while (end_server == false);
    destroy(pool);
    close(Main_Socket);
    return 0;
}

int init_pool(conn_pool_t * pool) { //Init the conn_pool_t structure.
    if (!pool) {
        printf("Allocation failure: \n");
        return FAILURE;
    }
    pool -> maxfd = 0;
    pool -> nready = 0;
    FD_ZERO( & pool -> read_set);
    FD_ZERO( & pool -> write_set);
    pool -> conn_head = NULL;
    pool -> nr_conns = 0;
    return SUCCESS;
}

int add_conn(int sd, conn_pool_t * pool) { //Add connection when new client connects the server.
    if (pool -> maxfd < sd) {
        pool -> maxfd = sd;
    }
    conn_t * conn = (conn_t * ) calloc(1, sizeof(conn_t));
    if (!conn) {
        printf("Allocation failure: \n");
        return FAILURE;
    }
    conn -> fd = sd;
    conn -> write_msg_head = conn -> write_msg_tail = NULL;
    if (!pool -> conn_head) {
        pool -> conn_head = conn;
        conn -> next = NULL;
        conn -> prev = NULL;
    } else {
        conn -> next = pool -> conn_head;
        pool -> conn_head -> prev = conn;
        pool -> conn_head = conn;
        pool -> conn_head -> prev = NULL;
    }
    pool -> nr_conns++;
    FD_SET(sd, & pool -> read_set);
    return SUCCESS;
}
void destroy(conn_pool_t * pool) { //free all the allocation that have been created
    struct conn * Cur = pool -> conn_head;
    struct conn * Prev;
    struct msg * Cur_msg, * Prev_msg;
    while (Cur != NULL) {
        printf("removing connection with sd %d \n", Cur -> fd);
        close(Cur -> fd);
        Cur_msg = Cur -> write_msg_head;
        while (Cur_msg != NULL) {
            Prev_msg = Cur_msg;
            Cur_msg = Cur_msg -> next;
            free(Prev_msg -> message);
            free(Prev_msg);
        }
        Prev = Cur;
        Cur = Cur -> next;
        free(Prev);
    }
    free(pool);
}
int remove_conn(int sd, conn_pool_t * pool) { //Remove connection when a client closes connection, or clean memory if server stops.
    struct conn * Cur = pool -> conn_head, * Prev = pool -> conn_head;
    int Second_fd = 0;
    int found = FAILURE;
    while (Cur != NULL) {
        if (Cur -> fd == sd) {
            found = SUCCESS;
            break;
        }
        Prev = Cur;
        Cur = Cur -> next;
    }
    if (found == FAILURE) {
        return FAILURE;
    }
    struct msg * Cur_msg = Cur -> write_msg_head, * Prev_msg;
    while (Cur_msg != NULL) {
        Prev_msg = Cur_msg;
        Cur_msg = Cur_msg -> next;
        free(Prev_msg -> message);
        free(Prev_msg);
    }
    if (Cur == pool -> conn_head) {
        pool -> conn_head = pool -> conn_head -> next;
    } else {
        Prev -> next = Cur -> next;
        if (Cur -> next != NULL)
            Cur -> next -> prev = Prev;
    }
    close(Cur -> fd);
    free(Cur);
    pool -> nr_conns--;
    struct conn * Cur_Second_fd = pool -> conn_head;
    while (Cur_Second_fd != NULL) {
        if (Cur_Second_fd -> fd > Second_fd)
            Second_fd = Cur_Second_fd -> fd;
        Cur_Second_fd = Cur_Second_fd -> next;
    }
    if (pool -> conn_head != NULL)
        pool -> maxfd = Second_fd;
    if (pool -> conn_head == NULL)
        pool -> maxfd = Main_Socket;
    FD_CLR(sd, & pool -> read_set);
    FD_CLR(sd, & pool -> write_set);
    FD_CLR(sd, & pool -> ready_write_set);
    FD_CLR(sd, & pool -> ready_read_set);
    return SUCCESS;
}

int add_msg(int sd, char * buffer, int len, conn_pool_t * pool) { //Add msg to the queues of all connections (except of the origin).
    struct conn * Cur = pool -> conn_head;
    while (Cur != NULL) {
        if (Cur -> fd == sd) {
            Cur = Cur -> next;
            continue;
        }
        struct msg * Meg = (struct msg * ) calloc(1, sizeof(struct msg));
        if (!Meg) {
            printf("Allocation failure: \n");
            return FAILURE;
        }
        Meg -> message = (char * ) calloc(len + 1, sizeof(char));
        if (!Meg -> message) {
            free(Meg);
            printf("Allocation failure: \n");
            return FAILURE;
        }
        strcpy(Meg -> message, buffer);
        Meg -> size = len;
        if (Cur -> write_msg_head == NULL) {
            Cur -> write_msg_head = Meg;
            Cur -> write_msg_tail = Meg;
            Meg -> next = Meg -> prev = NULL;
        } else {
            Meg -> next = Cur -> write_msg_head;
            Cur -> write_msg_head -> prev = Meg;
            Meg -> prev = NULL;
            Cur -> write_msg_head = Meg;
        }
        FD_SET(Cur -> fd, & pool -> write_set);
        Cur = Cur -> next;
    }
    return SUCCESS;
}

int write_to_client(int sd, conn_pool_t * pool) { //Write msg to client,and free the msg after that.
    struct conn * Cur = pool -> conn_head;
    int found = FAILURE;
    while (Cur != NULL) {
        if (Cur -> fd == sd) {
            found = SUCCESS;
            break;
        }
        Cur = Cur -> next;
    }
    if (found == FAILURE) { // no in there
        return FAILURE;
    }
    struct msg * Cur_msg = Cur -> write_msg_tail, * Prev_msg;
    while (Cur_msg != NULL) {
        if (write(sd, Cur_msg -> message, Cur_msg -> size) < 0) {
            return FAILURE;
        }
        Prev_msg = Cur_msg;
        Cur_msg = Cur_msg -> prev;
        free(Prev_msg -> message);
        free(Prev_msg);
        Cur -> write_msg_tail = Cur_msg;
    }
    Cur -> write_msg_head = Cur -> write_msg_tail = NULL;
    FD_CLR(sd, & pool -> write_set);
    FD_CLR(sd, & pool -> ready_write_set);
    return SUCCESS;
}