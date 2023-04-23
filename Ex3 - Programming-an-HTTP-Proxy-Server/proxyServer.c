#define _GNU_SOURCE
#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <netdb.h>

#include <stdbool.h>

#include <sys/stat.h>

#include <linux/limits.h>

#include <fcntl.h>

#include "threadpool.h"

#define FAILURE (-1)
#define SUCCESS 0
#define Allocation_Failure (-2)
#define KILOBYTE 1024
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404
#define NOT_SUPPORTED 501
#define INTERNAL_ERROR 500
#define ERROR_FILE (-701)
#define FILE_Exists (700)
#define FILE_NOT_Found (-700)
int size_Of_File_Filter = 0;
char ** File_Filter = NULL;
int initialization(int port, int pool_size, int max_requests_number);//initialize the socket , setup the sever and the threadpool start  working, and receive the clients
int IsDigits(const char * num);//check if the string has only numbers
int checkArguments(char * argv[], int * port, int * pool_size, int * max_requests_number);//check if the argument is legal or not if it is not legal returns failed , read the filter file
int StringToInt(char num[]);// this method turn the String to int
int Create_server(struct sockaddr_in * my_server, int * FD_Server, int port);//the function create the server and setup
int Dispatch_Client(void * arg);// function that busy working on clients
int read_from_socket(int fd_client, char ** request_received);//read the request from the client
int check_request(char * request_received, char ** path, char ** Host, int * code);//checks if the client request is proper with no errors
int send_error_response(int fd_client, int code);//send error message if was an error while checking
char * code_to_string(int code);//convert the error to a string
unsigned int Convert_to_binary(char * t, int num);//converting the ip address to binary
int check_is_in_the_filter(char * t,const char * Host);//check if the host name was inside the filter file
int Is_Path_Exist(char * Host, char * Path, bool O_CREAT_Boolean);// checks if path exists, if not it creates one according if boolean is true
int GetFromProxy(int fd_client, char * Host, char * Path, int * code);// gets information from the files
char * get_mime_type(char * name);//give the type request (given function)
int external_server(int fd_client, char * request_received, char * Host, char * Path);//connection to the socket and sends permission to it, writes inside the file the response of the server
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
int checkArguments(char * argv[], int * port, int * pool_size, int * max_requests_number) { //check if the argument is legal or not if it is not legal returns failed , read the filter file
    for (int i = 1; i < 4; i++) {
        if (IsDigits(argv[i]) == FAILURE)
            return FAILURE;
    }
    * port = StringToInt(argv[1]);
    * pool_size = StringToInt(argv[2]);
    * max_requests_number = StringToInt(argv[3]);
    if ( * pool_size > MAXT_IN_POOL || * pool_size <= 0) return FAILURE;
    if ( * max_requests_number < 1) return FAILURE;
    FILE * fp = fopen(argv[4], "r");
    if (fp == NULL) return FAILURE;
    char * linePtr = NULL;
    size_t size = 0;
    while (getline( & linePtr, & size, fp) != -1) {
        if (strcmp(linePtr, "\n") == 0 || strcmp(linePtr, "\r\n") == 0) continue;
        if (linePtr[strlen(linePtr) - 1] == '\n') {
            linePtr[strlen(linePtr) - 1] = '\0';
            if (linePtr[strlen(linePtr) - 1] == '\r') {
                linePtr[strlen(linePtr) - 1] = '\0';
            }
        }
        File_Filter = (char ** ) realloc(File_Filter, sizeof(char * ) * (size_Of_File_Filter + 1));
        if (File_Filter == NULL) {
            fprintf(stdout, "Allocation failure: \n");
            free(linePtr);
            fclose(fp);
            return Allocation_Failure;
        }
        File_Filter[size_Of_File_Filter] = (char * ) malloc(sizeof(char) * (strlen(linePtr) + 1));
        if (File_Filter[size_Of_File_Filter] == NULL) {
            fprintf(stdout, "Allocation failure: \n");
            free(linePtr);
            fclose(fp);
            for (int i = 0; i < size_Of_File_Filter; i++) {
                if (File_Filter[i] != NULL) {
                    free(File_Filter[i]);
                }
            }
            free(File_Filter);
            return Allocation_Failure;
        }
        strcpy(File_Filter[size_Of_File_Filter], linePtr);
        bzero(linePtr, size);
        size_Of_File_Filter++;
    }
    fclose(fp);
    if (linePtr != NULL) {
        free(linePtr);
    }
    return SUCCESS;
}
int initialization(int port, int pool_size, int max_requests_number) { //initialize the socket , setup the sever and the threadpool start  working, and receive the clients
    int FD_Server, FD_client, * Save_client_socket, i, cheak;
    struct sockaddr_in my_server, client_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    bzero((char * ) & my_server, sizeof(struct sockaddr_in));
    if (Create_server( & my_server, & FD_Server, port) == FAILURE) {
        for (int j = 0; j < size_Of_File_Filter; j++) {
            if (File_Filter[j] != NULL)
                free(File_Filter[j]);
        }
        free(File_Filter);
        exit(EXIT_FAILURE);
    }
    threadpool * threadpool = create_threadpool(pool_size);
    if (!threadpool) {
        for (int j = 0; j < size_Of_File_Filter; j++) {
            if (File_Filter[j] != NULL)
                free(File_Filter[j]);
        }
        free(File_Filter);
        cheak = close(FD_Server);
        if (cheak < 0) {
            perror("error: close\n");
        }
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < max_requests_number; i++) {
        bzero((char * ) & client_addr, sizeof(struct sockaddr_in));
        FD_client = accept(FD_Server, (struct sockaddr * ) & client_addr, & socklen);
        if (FD_client < 0) {
            perror("error: accept\n");
            continue;
        }
        Save_client_socket = (int * ) calloc(1, sizeof(int));
        if (Save_client_socket == NULL) {
            fprintf(stdout, "Allocation failure: ");
            cheak = close(FD_client);
            if (cheak < 0) {
                perror("error: close\n");
            }
            continue;
        }
        * Save_client_socket = FD_client;
        dispatch(threadpool, Dispatch_Client, (void * ) Save_client_socket);
    }
    cheak = close(FD_Server);
    if (cheak < 0) {
        perror("error: close\n");
    }
    destroy_threadpool(threadpool);
    for (int j = 0; j < size_Of_File_Filter; j++) {
        if (File_Filter[j] != NULL)
            free(File_Filter[j]);
    }
    free(File_Filter);
    return SUCCESS;
}
int Dispatch_Client(void * arg) { // function that busy working on clients
    int fd_client = * ((int * )(arg));
    free(arg);
    int code = 0, check;
    char * request_received = (char * ) calloc(201, sizeof(char));
    if (request_received == NULL) {
        printf("Allocation failure: \n");
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    memset(request_received, '\0', 201);
    check = read_from_socket(fd_client, & request_received);
    if (check < 0) {
        free(request_received);
        send_error_response(fd_client, INTERNAL_ERROR);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    if (strlen(request_received) == 0) {
        free(request_received);
        send_error_response(fd_client, INTERNAL_ERROR);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    int size = (int) strlen(request_received) + (int) strlen("index.html") + 1;
    char * path = (char * ) calloc(size, sizeof(char));
    if (!path) {
        printf("Allocation failure: \n");
        free(request_received);
        send_error_response(fd_client, INTERNAL_ERROR);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    char * Host = (char * ) calloc(strlen(request_received) + strlen("index.html") + 1, sizeof(char));
    if (!Host) {
        free(path);
        free(request_received);
        printf("Allocation failure: \n");
        send_error_response(fd_client, INTERNAL_ERROR);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    memset(path, '\0', size);
    memset(Host, '\0', strlen(request_received) + strlen("index.html") + 1);
    if (check_request(request_received, & path, & Host, & code) < 0) {
        send_error_response(fd_client, code);
        free(path);
        free(Host);
        free(request_received);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    printf("HTTP request =\n%s\nLEN = %d\n", request_received, (int) strlen(request_received));
    check = Is_Path_Exist(Host, path, false);
    if (check == ERROR_FILE) {
        send_error_response(fd_client, INTERNAL_ERROR);
        free(path);
        free(Host);
        free(request_received);
        check = close(fd_client);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    if (check == FILE_Exists) {
        if (GetFromProxy(fd_client, Host, path, & code) < 0) {
            send_error_response(fd_client, code);
            free(path);
            free(Host);
            free(request_received);
            check = close(fd_client);
            if (check < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
    }
    if (check == FILE_NOT_Found) {
        if (external_server(fd_client, request_received, Host, path) < 0) {
            send_error_response(fd_client, INTERNAL_ERROR);
            free(path);
            free(request_received);
            free(Host);
            check = close(fd_client);
            if (check < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
    }
    free(path);
    free(Host);
    free(request_received);
    check = close(fd_client);
    if (check < 0) {
        perror("error: close\n");
        return FAILURE;
    }
    return SUCCESS;
}
int external_server(int fd_client, char * request_received, char * Host, char * Path) { //connection to the socket and sends permission to it, writes inside the file the response of the server
    int Fd_external_server, check;
    if ((Fd_external_server = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error: socket\n");
        return FAILURE;
    }
    struct hostent * hp;
    struct sockaddr_in srv;
    bzero((char * ) & srv, sizeof(struct sockaddr_in));
    srv.sin_family = AF_INET;
    hp = gethostbyname(Host);
    if (!hp) {
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    srv.sin_addr.s_addr = ((struct in_addr * )(hp -> h_addr)) -> s_addr;
    srv.sin_port = htons(80);
    if (connect(Fd_external_server, (struct sockaddr * ) & srv, sizeof(srv)) < 0) {
        perror("error: connect\n");
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    int position = 0, size = 0;
    for (int i = 0; request_received[i] != '\0'; i++) {
        if (request_received[i] == '\r' && request_received[i + 1] == '\n') {
            position = i + 2;
            break;
        }
    }
    size = position + (int) strlen("Host: ") + (int) strlen(Host) + (int) strlen("\r\nConnection: close\r\n\r\n") + 1;
    char * new_request = (char * ) calloc(size, sizeof(char));
    if (new_request == NULL) {
        printf("Allocation failure: \n");
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    memset(new_request, '\0', size);
    strncpy(new_request, request_received, position);
    strcat(new_request, "Host: ");
    strcat(new_request, Host);
    strcat(new_request, "\r\nConnection: close\r\n\r\n");
    if (write(Fd_external_server, new_request, strlen(new_request)) < 0) {
        perror("error: write\n");
        free(new_request);
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    printf("File is given from origin server\n");
    char * total_Path = (char * ) calloc(1, (strlen(Host) + strlen(Path) + 1));
    if (total_Path == NULL) {
        printf("Allocation failure: \n");
        free(new_request);
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    memset(total_Path, '\0', (strlen(Host) + strlen(Path) + +1));
    sprintf(total_Path, "%s%s", Host, Path);
    int Sum_bytes = 0;
    int result;
    unsigned char * Socket_response = (unsigned char * ) malloc(sizeof(unsigned char) * 201);
    if (Socket_response == NULL) {
        printf("Allocation failure: \n");
        free(new_request);
        free(total_Path);
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return Allocation_Failure;
    }
    memset(Socket_response, 0, 201);
    unsigned char * Socket_response_buffer = NULL;
    int result_buffer = 0;
    int Print_in_file = true;
    while (1) {
        result = (int) read(Fd_external_server, Socket_response + Sum_bytes, 200);
        if (result == 0) break;
        if (result < 0) {
            free(total_Path);
            free(Socket_response);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
        Sum_bytes += result;
        Socket_response[Sum_bytes] = 0;
        if (strstr((const char * ) Socket_response, "\r\n\r\n") != NULL) {
            if (strstr((const char * ) Socket_response, "200 OK\r\n") != NULL) {
                Socket_response_buffer = (unsigned char * ) strstr((const char * ) Socket_response, "\r\n\r\n") + strlen("\r\n\r\n");
                for (int i = 0; i < Sum_bytes; i++) {
                    if (Socket_response[i] == '\r' && Socket_response[i + 1] == '\n' && Socket_response[i + 2] == '\r' && Socket_response[i + 3] == '\n') {
                        result_buffer = i + 4;
                    }
                }
            } else Socket_response_buffer = NULL;
            break;
        }
        Socket_response = (unsigned char * ) realloc(Socket_response, sizeof(unsigned char) * (200 + Sum_bytes + 1));
        if (Socket_response == NULL) {
            printf("Allocation failure: \n");
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            return Allocation_Failure;
        }
    }
    if (Socket_response_buffer == NULL) {
        Print_in_file = false;
    }
    if (write(fd_client, Socket_response, Sum_bytes) < 0) { //Send bytes
        perror("error: write\n");
        free(Socket_response);
        free(total_Path);
        free(new_request);
        check = close(Fd_external_server);
        if (check < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    int fd_file = -1;
    if (Print_in_file) {
        Is_Path_Exist(Host, Path, true);
        fd_file = open(total_Path, O_RDWR | O_CREAT, 0777);
        if (fd_file == -1) {
            perror("error: open\n");
            free(Socket_response);
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
        if (write(fd_file, Socket_response_buffer, Sum_bytes - result_buffer) < 0) {
            perror("error: write\n");
            free(Socket_response);
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            check = close(fd_file);
            if (check < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
    }
    unsigned char Read[200];
    while (1) {
        result = (int) read(Fd_external_server, Read, 200);
        if (result == 0) break;
        if (result < 0) {
            perror("error: read\n");
            free(Socket_response);
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            if (Print_in_file && close(fd_file) < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
        if (write(fd_client, Read, result) < 0) { //Send bytes
            perror("error: write\n");
            free(Socket_response);
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            if (Print_in_file && close(fd_file) < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
        if (Print_in_file && write(fd_file, Read, result) < 0) { //Send bytes
            perror("error: write\n");
            free(Socket_response);
            free(total_Path);
            free(new_request);
            check = close(Fd_external_server);
            if (check < 0) {
                perror("error: close\n");
            }
            if (Print_in_file && close(fd_file) < 0) {
                perror("error: close\n");
            }
            return FAILURE;
        }
        Sum_bytes += result;
    }
    printf("\n Total response bytes: %d\n", Sum_bytes);
    check = close(Fd_external_server);
    if (check < 0) {
        perror("error: close\n");
        free(Socket_response);
        free(total_Path);
        free(new_request);
        if (Print_in_file && close(fd_file) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    if (Print_in_file && close(fd_file) < 0) {
        perror("error: close\n");
        free(Socket_response);
        free(total_Path);
        free(new_request);
        return FAILURE;
    }
    free(total_Path);
    free(Socket_response);
    free(new_request);
    return SUCCESS;
}
int Is_Path_Exist(char * Host, char * Path, bool O_CREAT_Boolean) { // checks if path exists, if not it creates one according if boolean is true
    char total_Path[2 * PATH_MAX];
    bzero(total_Path, 2 * PATH_MAX);
    sprintf(total_Path, "%s%s", Host, Path);
    char save_Path[2 * PATH_MAX];
    bzero(save_Path, 2 * PATH_MAX);
    strcpy(save_Path, "");
    int creat = FILE_Exists;
    int countSlash = 0;
    for (int i = 0; total_Path[i] != '\0'; i++) {
        if (total_Path[i] == '/' && total_Path[i + 1] != '/' && total_Path[i + 1] != '\0')
            countSlash++;
    }
    char * token = strtok(total_Path, "/");
    for (; token != NULL; countSlash--) {
        strcat(save_Path, token);
        if (access(save_Path, F_OK) != 0) {
            creat = FILE_NOT_Found;
            if (O_CREAT_Boolean && countSlash != 0 && mkdir(save_Path, 0700) != 0) {
                perror("error: mkdir\n");
                return ERROR_FILE;
            }
        }
        strcat(save_Path, "/");
        token = strtok(NULL, "/");
    }
    return creat;
}
int GetFromProxy(int fd_client, char * Host, char * Path, int * code) {// gets information from the files
    char total_Path[2 * PATH_MAX];
    bzero(total_Path, 2 * PATH_MAX);
    sprintf(total_Path, "%s%s", Host, Path);
    int fd_file = open(total_Path, O_RDWR | O_CREAT);
    if (fd_file == -1) {
        perror("error: open\n");
        * code = INTERNAL_ERROR;
        return FAILURE;
    }
    int File_size = (int) lseek(fd_file, 0, SEEK_END);
    lseek(fd_file, 0, SEEK_SET);
    printf("File is given from local filesystem\r\n");
    char header[KILOBYTE] = {0};
    char * type = get_mime_type(total_Path);
    if (!type) {
        sprintf(header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", File_size);
    } else {
        sprintf(header, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",type, File_size);
    }
    if (write(fd_client, header, strlen(header)) < 0) {
        * code = INTERNAL_ERROR;
        return FAILURE;
    }
    unsigned char Read[200];
    int bytes, SumOfBytes = 0;
    while (1) {
        bytes = (int) read(fd_file, Read, 200);
        if (bytes == 0) break;
        if (bytes < 0) {
            perror("error: read\n");
            * code = INTERNAL_ERROR;
            return FAILURE;
        }
        if (write(fd_client, Read, bytes) < 0) {
            * code = INTERNAL_ERROR;
            return FAILURE;
        }
        SumOfBytes += bytes;
    }
    printf("\n Total response bytes: %d\n", (int) strlen(header) + SumOfBytes);
    if (close(fd_file) == -1) {
        * code = INTERNAL_ERROR;
        return FAILURE;
    }
    return SUCCESS;
}

int check_request(char * request_received, char ** path, char ** Host, int * code) { //checks if the client request is proper with no errors
    if(strstr(request_received,"\r\n\r\n")==NULL){
        * code = BAD_REQUEST;
        return FAILURE;
    }
    char * Method = (char * ) calloc(strlen(request_received) + 1, sizeof(char));
    if (!Method) {
        printf("Allocation failure: \n");
        * code = INTERNAL_ERROR;
        return Allocation_Failure;
    }
    memset(Method, '\0', strlen(request_received) + 1);
    char * Protocol_HTTP_Version_0 = strstr(request_received, "HTTP/1.0");
    char * Protocol_HTTP_Version_1 = strstr(request_received, "HTTP/1.1");
    if ((!Protocol_HTTP_Version_0 && !Protocol_HTTP_Version_1) || (Protocol_HTTP_Version_0 && Protocol_HTTP_Version_1)) {
        * code = BAD_REQUEST;
        free(Method);
        return FAILURE;
    }
    if (Protocol_HTTP_Version_0) {
        sscanf(request_received, "%s %s HTTP/1.0\r\n", Method, * path);
        if (strlen(Method) == 0 || strlen( * path) == 0 || ( * path)[0] != '/' || strcmp(Method,"HTTP/1.0")==0|| strcmp((*path),"HTTP/1.0")==0||strcmp(Method,"/HTTP/1.0")==0|| strcmp((*path),"/HTTP/1.0")==0) {
            * code = BAD_REQUEST;
            free(Method);
            return FAILURE;
        }
        Protocol_HTTP_Version_0 = Protocol_HTTP_Version_0 + strlen("HTTP/1.0");
        while (Protocol_HTTP_Version_0 != NULL && Protocol_HTTP_Version_0[0] == ' ') {
            Protocol_HTTP_Version_0 = Protocol_HTTP_Version_0 + 1;
        }
        if (Protocol_HTTP_Version_0 == NULL || strlen(Protocol_HTTP_Version_0) < 2 || Protocol_HTTP_Version_0[0] != '\r' || Protocol_HTTP_Version_0[1] != '\n') {
            * code = BAD_REQUEST;
            free(Method);
            return FAILURE;
        }
    }
    if (Protocol_HTTP_Version_1) {
        sscanf(request_received, "%s %s HTTP/1.1\r\n", Method, * path);
        if (strlen(Method) == 0 || strlen( * path) == 0 || ( * path)[0] != '/' || strcmp(Method,"HTTP/1.1")==0|| strcmp((*path),"HTTP/1.1")==0||strcmp(Method,"/HTTP/1.1")==0|| strcmp((*path),"/HTTP/1.1")==0){
            * code = BAD_REQUEST;
            free(Method);
            return FAILURE;
        }
        Protocol_HTTP_Version_1 = Protocol_HTTP_Version_1 + strlen("HTTP/1.1");
        while (Protocol_HTTP_Version_1 != NULL && Protocol_HTTP_Version_1[0] == ' ') {
            Protocol_HTTP_Version_1 = Protocol_HTTP_Version_1 + 1;
        }
        if (Protocol_HTTP_Version_1 == NULL || strlen(Protocol_HTTP_Version_1) < 2 || Protocol_HTTP_Version_1[0] != '\r' || Protocol_HTTP_Version_1[1] != '\n') {
            * code = BAD_REQUEST;
            free(Method);
            return FAILURE;
        }
    }
    if (strcasestr(request_received, "Host:") == NULL) {
        * code = BAD_REQUEST;
        free(Method);
        return FAILURE;
    }
    char * check_request_received = strcasestr(request_received, "Host:") + strlen("Host:");
    if (check_request_received == NULL || strlen(check_request_received) == 0) {
        free(Method);
        * code = BAD_REQUEST;
        return FAILURE;
    }
    while (check_request_received != NULL && check_request_received[0] == ' ') {
        check_request_received = check_request_received + 1;
    }
    if (check_request_received == NULL) {
        free(Method);
        * code = BAD_REQUEST;
        return FAILURE;
    }
    sscanf(check_request_received, "%s\r\n", * Host);
    if((*Host)==NULL|| strlen(*Host)==0){
        free(Method);
        * code = BAD_REQUEST;
        return FAILURE;
    }
    char * tmp = strstr(( * Host), ":");
    if (tmp != NULL) {
        tmp[0] = '\0';
    }
    if (strncmp(request_received, "GET ", 4) != 0) {
        * code = NOT_SUPPORTED;
        free(Method);
        return FAILURE;
    }
    struct hostent * hp;
    hp = gethostbyname( * Host);
    if (!hp) {
        * code = NOT_FOUND;
        free(Method);
        return FAILURE;
    }
    if (( * path)[strlen(( * path)) - 1] == '/') {
        strcat(( * path), "index.html");
    }
    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = ((struct in_addr * )(hp -> h_addr)) -> s_addr;
    char * t = inet_ntoa(srv.sin_addr);
    if (t == 0) {
        * code = NOT_FOUND;
        free(Method);
        return FAILURE;
    }
    int check = check_is_in_the_filter(t, * Host);
    if (check < 0) {
        if (check == FAILURE)
            *code = FORBIDDEN;
        if (check == Allocation_Failure)
            *code = INTERNAL_ERROR;
        free(Method);
        return FAILURE;
    }
    if (strstr( * path, "%20") == NULL) {
        free(Method);
        return SUCCESS;
    }
    int save = (int) strlen( * path) + 1;
    char * tmp_Path = (char * ) calloc(save, sizeof(char));
    if (!tmp_Path) {
        printf("Allocation failure: \n");
        * code = INTERNAL_ERROR;
        return Allocation_Failure;
    }
    memset(tmp_Path, '\0', strlen( * path) + 1);
    int i, j;
    for (i = 0, j = 0;
         ( * path)[i] != '\0'; i++, j++) {
        if (strncmp( & (( * path)[i]), "%20", 3) == 0) {
            i++;
            i++;
            tmp_Path[j] = ' ';
        } else {
            tmp_Path[j] = ( * path)[i];
        }
    }
    bzero( * path, strlen(request_received) + 1);
    strcpy( * path, tmp_Path);
    bzero(tmp_Path, save);
    free(Method);
    free(tmp_Path);
    return SUCCESS;
}
unsigned int Convert_to_binary(char * t, int num) { //converting the ip address to binary
    unsigned int binary = 0, tmp = 0, arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;
    sscanf(t, "%u.%u.%u.%u", & arg1, & arg2, & arg3, & arg4);
    binary = (binary | arg1) << 8;
    binary = (binary | arg2) << 8;
    binary = (binary | arg3) << 8;
    binary = binary | arg4;
    for (int i = 0; i < (32 - num); i++) {
        tmp = tmp | 1;
        if (i != 32 - num - 1)
            tmp = tmp << 1;
    }
    tmp = tmp << num;
    binary = (binary & tmp);
    return binary >> num;
}
int check_is_in_the_filter(char * t,const char * Host) { //check if the host name was inside the filter file
    unsigned int IP_Host;
    unsigned int IP_Filter;
    char tmp[16] = {0};
    strcpy(tmp, t);
    int num;
    struct hostent * he;
    char * txt_num = NULL;
    bool I_HAVE_IP_IN_Request = false;
    struct in_addr ** addr_list;
    if (Host[0] >= '0' && Host[1] <= '9') {
        I_HAVE_IP_IN_Request = true;
    }
    for (int j = 0; j < size_Of_File_Filter; j++) {
        strcpy(t, tmp);
        if (File_Filter[j][0] >= '0' && File_Filter[j][0] <= '9') {
            if (strstr(File_Filter[j], "/") == NULL) {
                IP_Filter = Convert_to_binary(File_Filter[j], 0);
                IP_Host = Convert_to_binary(t, 0);
                if (IP_Filter == IP_Host) {
                    return FAILURE;
                }
            } else {
                txt_num = strstr(File_Filter[j], "/") + 1;
                if (txt_num == NULL || strlen(txt_num) == 0 || IsDigits(txt_num) == FAILURE) {
                    return FAILURE;
                }
                num = StringToInt(txt_num);
                IP_Filter = Convert_to_binary(File_Filter[j], 32 - num);
                IP_Host = Convert_to_binary(t, 32 - num);
                if (IP_Filter == IP_Host) {
                    return FAILURE;
                }
            }
        } else {
            if (I_HAVE_IP_IN_Request == true) {
                IP_Host = Convert_to_binary(t, 0);
                he = gethostbyname(File_Filter[j]);
                if (he == NULL) {
                    herror("gethostbyname");
                    return FAILURE;
                }
                addr_list = (struct in_addr ** ) he -> h_addr_list;
                for (int i = 0; addr_list[i] != NULL; i++) {
                    IP_Filter = Convert_to_binary(inet_ntoa( * addr_list[i]), 0);
                    if (IP_Filter == IP_Host) {
                        return FAILURE;
                    }
                }
            } else {
                if (strcmp(Host, File_Filter[j]) == 0) {
                    return FAILURE;
                }
            }
        }
    }
    strcpy(t, tmp);
    bzero(tmp, 16);
    return SUCCESS;
}
int send_error_response(int fd_client, int code) {//send error message if was an error while checking
    char body[KILOBYTE / 2] = {0};
    char headers[KILOBYTE / 2] = {0};
    char response[KILOBYTE] = {0};
    char * string_code = code_to_string(code);
    sprintf(body,
            "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n<BODY><H4>%s</H4>\r\n%s\r\n</BODY></HTML>\r\n",
            string_code, string_code,
            code == BAD_REQUEST ? "Bad Request." :
            code == FORBIDDEN ? "Access denied." :
            code == NOT_FOUND ? "File not found." :
            code == INTERNAL_ERROR ? "Some server side error." : "Method is not supported.");
    sprintf(headers, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n", string_code, strlen(body));
    sprintf(response, "%s%s", headers, body);
    if (write(fd_client, response, strlen(response)) < 0) {
        perror("error: write\n");
        return FAILURE;
    }
    return SUCCESS;
}
char * code_to_string(int code) { //convert the error to a string
    if (code == 200)
        return "200 OK";
    else if (code == 400)
        return "400 Bad Request";
    else if (code == 403)
        return "403 Forbidden";
    else if (code == 404)
        return "404 Not Found";
    else if (code == 500)
        return "500 Internal Server Error";
    else
        return "501 Not supported";
}
char * get_mime_type(char * name) { //give the type request (given function)
    char * ext = strrchr(name, '.');
    if (!ext)
        return NULL;
    if (!strcmp(ext, ".html") || !strcmp(ext, ".htm") || !strcmp(ext, ".txt"))
        return "text/html";
    if (!strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg"))
        return "image/jpeg";
    if (!strcmp(ext, ".gif"))
        return "image/gif";
    if (!strcmp(ext, ".png"))
        return "image/png";
    if (!strcmp(ext, ".css"))
        return "text/css";
    if (!strcmp(ext, ".au"))
        return "audio/basic";
    if (!strcmp(ext, ".wav"))
        return "audio/wav";
    if (!strcmp(ext, ".avi"))
        return "video/x-msvideo";
    if (!strcmp(ext, ".mpeg") || !strcmp(ext, ".mpg"))
        return "video/mpeg";
    if (!strcmp(ext, ".mp3"))
        return "audio/mpeg";
    return NULL;
}
int read_from_socket(int fd_client, char ** request_received) { //read the request from the client
    int result, Sum_bytes = 0;
    while (1) {
        result = (int) read(fd_client, ( * request_received) + Sum_bytes, 200);
        if (result == 0) {
            ( * request_received)[Sum_bytes] = '\0';
            break;
        }
        if (result < 0) {
            ( * request_received)[Sum_bytes] = '\0';
            perror("error: read\n");
            return FAILURE;
        }
        Sum_bytes += result;
        ( * request_received)[Sum_bytes] = '\0';
        if (strstr(( * request_received), "\r\n\r\n") != NULL) break;
        ( * request_received) = (char * ) realloc(( * request_received), sizeof(char) * (200 + Sum_bytes + 1));
        if (!( * request_received)) {
            printf("Allocation failure: \n");
            return Allocation_Failure;
        }
    }
    ( * request_received)[Sum_bytes] = '\0';
    return SUCCESS;
}
int Create_server(struct sockaddr_in * my_server, int * FD_Server, int port) {//the function create the server and setup
    my_server -> sin_family = AF_INET;
    my_server -> sin_addr.s_addr = INADDR_ANY;
    my_server -> sin_port = htons(port);
    * FD_Server = socket(AF_INET, SOCK_STREAM, 0);
    if ( * FD_Server < 0) {
        perror("error: socket\n");
        return FAILURE;
    }
    if (bind( * FD_Server, (struct sockaddr * ) my_server, sizeof(struct sockaddr_in)) < 0) {
        perror("error: bind\n");
        if (close( * FD_Server) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    if (listen( * FD_Server, 10) < 0) {
        perror("error: listen\n");
        if (close( * FD_Server) < 0) {
            perror("error: close\n");
        }
        return FAILURE;
    }
    return SUCCESS;
}
int main(int argc, char * argv[]) {//call the initialization function and check Arguments
    if (argc != 5) {
        printf("Usage: server <port> <pool-size> <max-requests-number> <filter>\n");
        exit(EXIT_FAILURE);
    }
    int port, pool_size, max_requests_number;
    int check;
    check = checkArguments(argv, & port, & pool_size, & max_requests_number);
    if (check == FAILURE) {
        printf("Usage: server <port> <pool-size> <max-requests-number> <filter>\n");
        for (int j = 0; j < size_Of_File_Filter; j++) {
            free(File_Filter[j]);
        }
        free(File_Filter);
        exit(EXIT_FAILURE);
    }
    if (check == Allocation_Failure) {
        printf("Allocation failure: \n");
        for (int j = 0; j < size_Of_File_Filter; j++) {
            free(File_Filter[j]);
        }
        free(File_Filter);
        exit(EXIT_FAILURE);
    }
    initialization(port, pool_size, max_requests_number);
    return 0;
}