#include <stdio.h>

#include <string.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/socket.h>

#include <stdbool.h>

#include <netdb.h>

#include <arpa/inet.h>

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
int CountCharInString(const char * A, char c) { //counts how many times char c is in string A
    int i, count = 0;
    for (i = 0; A[i] != '\0'; i++) {
        if (A[i] == c) count++;
    }
    return count;
}
bool isCompareToSpaceOrBlank(const char * A) { //checks if there is empty space in the string
    if (A == NULL) return true;
    int i;
    for (i = 0; A[i] != '\0'; i++) {
        if (A[i] != ' ') return false;
    }
    return true;
}
bool IsASlashSpace(const char * A) { //checks if the string only contains spaces and dashes
    int i;
    for (i = 0; A[i] != '\0'; i++) {
        if (A[i] != ' ' && A[i] != '/')
            return false;
    }
    return true;
}
void Free(char ** AllTheWordInThePath, char * Path, char * interface, char * Host, int lengthOfThePath) { //free all allocation memory
    for (int i = 0; AllTheWordInThePath != NULL && i < lengthOfThePath; i++) {
        if (AllTheWordInThePath[i] != NULL)
            free(AllTheWordInThePath[i]);
    }
    if (AllTheWordInThePath != NULL)
        free(AllTheWordInThePath);
    if (Path != NULL)
        free(Path);
    if (interface != NULL)
        free(interface);
    if (Host != NULL)
        free(Host);
}
char * separated(const char * A, char c, char ** AllTheWordInThePath, char * Path, char * interface, char * Host, int lengthOfThePath) {
    // divides the string until it reaches the letter c
    if (strchr(A, c) == NULL) NULL;
    int i;
    for (i = 0; A[i] != '\0' && A[i] != c; i++);
    char * separated = (char * ) malloc(sizeof(char) * (i + 1));
    if (separated == NULL) {
        printf("Allocation Failed");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        exit(EXIT_FAILURE);
    }
    for (i = 0; A[i] != '\0' && A[i] != c; i++) separated[i] = A[i];
    separated[i] = '\0';
    return separated;
}
char ** separatedThePath(char ** Path, char * Host, char * result, char * MiniPath, char ** AllTheWordInThePath, int * lengthOfThePath, char * interface) {
    // divides the path to sub-strings according to dashes
    * lengthOfThePath = 1;
    AllTheWordInThePath = (char ** ) malloc(sizeof(char * ) * ( * lengthOfThePath));
    if (AllTheWordInThePath == NULL) {
        printf("Allocation Failed");
        Free(NULL, * Path, interface, Host, 0);
        exit(EXIT_FAILURE);
    }
    AllTheWordInThePath[ * lengthOfThePath - 1] = (char * ) malloc(sizeof(char) * (strlen(Host) + 1));
    if (AllTheWordInThePath[ * lengthOfThePath - 1] == NULL) {
        printf("Allocation Failed");
        Free(AllTheWordInThePath, NULL, interface, Host, * lengthOfThePath);
        exit(EXIT_FAILURE);
    }
    strcpy(AllTheWordInThePath[ * lengthOfThePath - 1], Host);
    * Path = (char * ) realloc( * Path, strlen(Host) + 1);
    if ( * Path == NULL) {
        printf("Allocation Failed");
        Free(AllTheWordInThePath, NULL, interface, Host, * lengthOfThePath);
        exit(EXIT_FAILURE);
    }
    strcpy( * Path, Host);
    if (strchr(result, '/') == NULL || (CountCharInString(result, '/') == 1 && isCompareToSpaceOrBlank((strchr(result, '/') + 1)))) {
        * Path = (char * ) realloc( * Path, strlen( * Path) + strlen("/") + strlen("index.html") + 1);
        strcat( * Path, "/");
        strcat( * Path, "index.html");
        ( * lengthOfThePath) ++;
        AllTheWordInThePath = (char ** ) realloc(AllTheWordInThePath, sizeof(char * ) * ( * lengthOfThePath));
        if (AllTheWordInThePath == NULL) {
            printf("Allocation Failed");
            Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath - 1);
            exit(EXIT_FAILURE);
        }
        AllTheWordInThePath[( * lengthOfThePath) - 1] = (char * ) malloc(sizeof(char) * (strlen("index.html") + 1));
        if (AllTheWordInThePath[( * lengthOfThePath) - 1] == NULL) {
            printf("Allocation Failed");
            Free(AllTheWordInThePath, * Path, interface, Host, ( * lengthOfThePath));
            exit(EXIT_FAILURE);
        }
        strcpy(AllTheWordInThePath[( * lengthOfThePath) - 1], "index.html");
    } else {
        while (strchr(result, '/') != NULL) {
            result = strchr(result, '/');
            if (IsASlashSpace(result) == true) { //if the end of the string only spaces & dashes add to path
                * Path = (char * ) realloc( * Path, strlen( * Path) + strlen(result) + 2);
                if ( * Path == NULL) {
                    printf("Allocation Failed");
                    Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath);
                    exit(EXIT_FAILURE);
                }
                strcat( * Path, result);
                ( * lengthOfThePath) ++;
                AllTheWordInThePath = (char ** ) realloc(AllTheWordInThePath, sizeof(char * ) * ( * lengthOfThePath));
                if (AllTheWordInThePath == NULL) {
                    printf("Allocation Failed");
                    Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath - 1);
                    exit(EXIT_FAILURE);
                }
                AllTheWordInThePath[( * lengthOfThePath) - 1] = (char * ) malloc(sizeof(char) * ((int) strlen("index.html") + 1));
                if (AllTheWordInThePath[( * lengthOfThePath) - 1] == NULL) {
                    printf("Allocation Failed");
                    Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath - 1);
                    exit(EXIT_FAILURE);
                }
                strcpy(AllTheWordInThePath[( * lengthOfThePath) - 1], "index.html");
                break;
            }
            result += 1;
            if (isCompareToSpaceOrBlank(result)) break;
            MiniPath = separated(result, '/', AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath);
            * Path = (char * ) realloc( * Path, strlen( * Path) + strlen(MiniPath) + 2);
            if ( * Path == NULL) {
                printf("Allocation Failed");
                Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath);
                exit(EXIT_FAILURE);
            }
            ( * lengthOfThePath) ++;
            AllTheWordInThePath = (char ** ) realloc(AllTheWordInThePath, sizeof(char * ) * ( * lengthOfThePath));
            if (AllTheWordInThePath == NULL) {
                printf("Allocation Failed");
                Free(AllTheWordInThePath, * Path, interface, Host, * lengthOfThePath - 1);
                exit(EXIT_FAILURE);
            }
            strcat( * Path, "/");
            strcat( * Path, MiniPath);
            AllTheWordInThePath[( * lengthOfThePath) - 1] = MiniPath;
        }
    }
    return AllTheWordInThePath;
}
bool Is_Path_Exist(int Strlen_Path, char ** AllTheWordInThePath, int lengthOfThePath, bool O_CREAT_Boolean, char * Path, char * interface, char * Host) {
    // checks if path exists, if not it creates one according if boolean is true
    struct stat check = {
            0
    };
    int i = 0;
    bool isExist = true;
    char chunk[Strlen_Path + 1];
    strcpy(chunk, "");
    strcpy(chunk, AllTheWordInThePath[i]);
    for (i = 1; i < lengthOfThePath; i++) {
        if (stat(chunk, & check) == -1) {
            isExist = false;
            if (O_CREAT_Boolean && mkdir(chunk, 0700) != 0) {
                perror("mkdir: Unable to create directory\n");
                Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
                exit(EXIT_FAILURE);
            }
        }
        strcat(chunk, "/");
        strcat(chunk, AllTheWordInThePath[i]);
    }
    if (stat(chunk, & check) == -1) isExist = false;
    return isExist;
}
void GetFromProxy(char * Path, char ** AllTheWordInThePath, char * port, char * Host, int lengthOfThePath, char * Use_Path) {
    // gets information from the files
    int fd_file = open(Use_Path, O_RDWR | O_CREAT);
    if (fd_file == -1) {
        perror("Open: failed to Open File");
        Free(AllTheWordInThePath, Path, port, Host, lengthOfThePath);
        exit(EXIT_FAILURE);
    }
    int File_size = (int) lseek(fd_file, 0, SEEK_END);
    lseek(fd_file, 0, SEEK_SET);
    unsigned char Read[200];
    printf("File is given from local filesystem\r\n");
    printf("HTTP/1.0 200 OK\r\n");
    printf("Content-Length: %d\r\n\r\n", File_size);
    int bytes, SumOfBytes = 0;
    while (1) {
        bytes = (int) read(fd_file, Read, 200);
        if (bytes == 0) break;
        if (bytes < 0) {
            perror("Read: Failed To Read");
            Free(AllTheWordInThePath, Path, port, Host, lengthOfThePath);
            exit(EXIT_FAILURE);
        }
        if (write(1, Read, bytes) < 0) {
            perror("Write: Failed To Write");
            Free(AllTheWordInThePath, Path, port, Host, lengthOfThePath);
            exit(EXIT_FAILURE);
        }
        SumOfBytes += bytes;
    }
    int Tmp_File_Size = File_size;
    int HowManyDighit = 0;
    do {
        Tmp_File_Size /= 10;
        ++HowManyDighit;
    } while (Tmp_File_Size != 0);
    printf("\r\nTotal response bytes:%d ", (SumOfBytes + (int) strlen("HTTP/1.0 200 OK\r\n") + (int) strlen("Content-Length: ") + HowManyDighit + (int) strlen("\r\n\r\n")));
    if (close(fd_file) == -1) {
        perror("Close: Close Connection With the Proxy is Failed");
        Free(AllTheWordInThePath, Path, port, Host, lengthOfThePath);
        exit(EXIT_FAILURE);
    }
}
void writeToFileFromSocket(int Fd_Socket, char * Path, char ** AllTheWordInThePath, int lengthOfThePath, char * interface, char * Host, char * Use_Path) {
    // writes inside the file the response of the server
    int Sum_bytes = 0;
    int result;
    unsigned char * Socket_response = (unsigned char * ) malloc(sizeof(unsigned char) * 201);
    memset(Socket_response, 0, 201);
    unsigned char * Socket_response_buffer = NULL;
    int result_buffer;
    int Print_in_file = true;
    while (1) {
        result = (int) read(Fd_Socket, Socket_response + Sum_bytes, 200);
        if (result == 0) break;
        if (result < 0) {
            perror("Read:Failed To Read");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
        Sum_bytes += result;
        Socket_response[Sum_bytes] = 0;
        if (strstr((const char * ) Socket_response, "\r\n\r\n") != NULL) {
            if (strstr((const char * ) Socket_response, "200 OK\r\n") != NULL) {
                Socket_response_buffer = (unsigned char * ) strstr((const char * ) Socket_response, "\r\n\r\n") + strlen("\r\n\r\n");
                result_buffer = Sum_bytes - (int)(Socket_response_buffer - Socket_response);
            } else Socket_response_buffer = NULL;
            break;
        }
        Socket_response = (unsigned char * ) realloc(Socket_response, sizeof(unsigned char) * (200 + Sum_bytes + 1));
    }
    if (Socket_response_buffer == NULL) {
        Print_in_file = false;
    }
    if (write(1, Socket_response, Sum_bytes) < 0) { //Send bytes
        perror("Write: Failed To Write");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(Socket_response);
        exit(EXIT_FAILURE);
    }
    int fd_file = -1;
    if (Print_in_file) {
        Is_Path_Exist((int) strlen(Path), AllTheWordInThePath, lengthOfThePath, true, Path, interface, Host);
        fd_file = open(Use_Path, O_RDWR | O_CREAT);
        if (fd_file == -1) {
            perror("Open: Failed Open");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
        if (write(fd_file, Socket_response_buffer, result_buffer) < 0) {
            perror("Write: Failed To Write");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
    }
    unsigned char Read[200];
    while (1) {
        result = (int) read(Fd_Socket, Read, 200);
        if (result == 0) break;
        if (result < 0) {
            perror("Read: Failed To Read");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
        if (write(1, Read, result) < 0) { //Send bytes
            perror("Write: Failed To write");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
        if (Print_in_file && write(fd_file, Read, result) < 0) { //Send bytes
            perror("Write: Failed To Write");
            Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
            free(Socket_response);
            exit(EXIT_FAILURE);
        }
        Sum_bytes += result;
    }
    printf("\r\nTotal response bytes: %d", Sum_bytes);
    if ((Print_in_file && close(fd_file) == -1) || close(Fd_Socket) == -1) {
        perror("Close: Failed To Close");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(Socket_response);
        exit(EXIT_FAILURE);
    }
    free(Socket_response);
}
void Socket(char * Host, char * Path, char * interface, char ** AllTheWordInThePath, int lengthOfThePath, bool thereAreNoSlashOrSlashInTheEnd, char * Use_Path) { //checks if the url is correct, checks if there is port, and calls on functions
    //connection to the socket and sends permission to it
    char * Path_Socket = strchr(Path, '/') + 1;
    int strlen_Path_Socket = 0;
    if (!thereAreNoSlashOrSlashInTheEnd) strlen_Path_Socket = (int) strlen(Path_Socket);
    char * String_req = (char * ) malloc(sizeof(char) * (strlen("GET /") + strlen_Path_Socket + strlen(" HTTP/1.0\r\nHost: ") +
                                                         strlen(Host) + strlen("\r\n\r\n") + 1));
    if (String_req == NULL) {
        printf("Allocation Failed");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        exit(EXIT_FAILURE);
    }
    strcpy(String_req, "");
    strcat(String_req, "GET /");
    if (!thereAreNoSlashOrSlashInTheEnd)
        strcat(String_req, Path_Socket);
    strcat(String_req, " HTTP/1.0\r\nHost: ");
    strcat(String_req, Host);
    strcat(String_req, "\r\n\r\n");
    printf("HTTP request =\n%sLEN = %d\n", String_req, (int) strlen(String_req));

    int Fd_socket; /* socket descriptor */
    if ((Fd_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(String_req);
        exit(EXIT_FAILURE);
    }
    struct hostent * hp; /*ptr to host info for remote*/
    struct sockaddr_in srv; /* used by connect() */
    srv.sin_family = AF_INET;
    hp = gethostbyname(Host);
    if (!hp) {
        herror("This is not usage error, you will fail when trying to get the IP address for that host.\n");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(String_req);
        exit(EXIT_FAILURE);
    }
    srv.sin_addr.s_addr = ((struct in_addr * )(hp -> h_addr)) -> s_addr;
    srv.sin_port = htons((unsigned short) StringToInt(interface));
    if (connect(Fd_socket, (struct sockaddr * ) & srv, sizeof(srv)) < 0) {
        perror("Connect");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(String_req);
        exit(EXIT_FAILURE);
    }
    if (write(Fd_socket, String_req, strlen(String_req)) < 0) {
        perror("Write: Failed To Write");
        Free(AllTheWordInThePath, Path, interface, Host, lengthOfThePath);
        free(String_req);
        exit(EXIT_FAILURE);
    }
    free(String_req);
    writeToFileFromSocket(Fd_socket, Path, AllTheWordInThePath, lengthOfThePath, interface, Host, Use_Path);
}
int main(int argc, char * argv[]) { //checks if the url is correct, checks if there is port, and calls on functions
    if (argc != 2) {
        printf("Usage:proxy1 <URL>\n");
        exit(EXIT_FAILURE);
    }
    if (strstr(argv[1], "http://") == NULL) {
        fprintf(stdout, "This is not usage error, you will fail when trying to get the IP address for that host");
        exit(EXIT_FAILURE);
    }
    char * result;
    char * Path = NULL;
    char * MiniPath = NULL;
    char * Host;
    char * interface;
    int lengthPartsOfPath = 0;
    char ** AllThePartInThePath = NULL;
    result = argv[1] + strlen("http://");
    bool thereAreNoSlashOrSlashInTheEnd = false;
    if (strchr(result, ':') == NULL&&(CountCharInString(result, '/') == 0 || CountCharInString(result, '/') == 1)) {
        thereAreNoSlashOrSlashInTheEnd = true;
    }
    if (strchr(result, ':') == NULL) {
        interface = (char * ) malloc(sizeof(char) * (strlen("80") + 1));
        strcpy(interface, "80");
        Host = separated(result, '/', NULL, NULL, interface, NULL, lengthPartsOfPath);
    } else {
        Host = separated(result, ':', NULL, NULL, NULL, NULL, lengthPartsOfPath);
        result = strchr(result, ':') + 1;
        if (result == NULL || * result > '9' || * result < '0') {
            fprintf(stdout, "Error No Port");
            Free(NULL, NULL, NULL, Host, 0);
            exit(EXIT_FAILURE);
        }
        interface = separated(result, '/', NULL, NULL, NULL, Host, lengthPartsOfPath);
        for (int i = 0; interface[i] != '\0'; i++)
            if (interface[i] > '9' || interface[i] < '0') {
                fprintf(stdout, "Error No Port");
                Free(NULL, NULL, interface, Host, 0);
                exit(EXIT_FAILURE);
            }
    }
    AllThePartInThePath = separatedThePath( & Path, Host, result, MiniPath, AllThePartInThePath, & lengthPartsOfPath, interface);
    char * Use_Path = (char * ) malloc(sizeof(char) * ((int) strlen(Path) + (int) strlen("index.html") + 1));
    if (Use_Path == NULL) {
        Free(AllThePartInThePath, Path, interface, Host, lengthPartsOfPath);
        exit(EXIT_FAILURE);
    }
    if (strstr(Path, "index.html") == NULL && Path[(int) strlen(Path) - 1] == '/') {
        sprintf(Use_Path, "%sindex.html", Path);
    } else {
        sprintf(Use_Path, "%s", Path);
    }
    bool D = Is_Path_Exist((int) strlen(Path), AllThePartInThePath, lengthPartsOfPath, false, Path, interface, Host);
    if (D == true) GetFromProxy(Path, AllThePartInThePath, interface, Host, lengthPartsOfPath, Use_Path);
    else Socket(Host, Path, interface, AllThePartInThePath, lengthPartsOfPath, thereAreNoSlashOrSlashInTheEnd, Use_Path);
    Free(AllThePartInThePath, Path, interface, Host, lengthPartsOfPath);
    free(Use_Path);
    return (0);
}