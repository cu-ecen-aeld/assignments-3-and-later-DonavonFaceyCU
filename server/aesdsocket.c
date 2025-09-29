/*

File: aesdsocket.c
Author: Donavon Facey

*/

#include <syslog.h>     // syslog()
#include <stdio.h>      // printf()
#include <stdbool.h>    // for bool
#include <unistd.h>     // close()
#include <stdlib.h>     // ?
#include <sys/types.h>  // ?
#include <sys/socket.h> // socket()
#include <netdb.h>      // getaddrinfo()
#include <sys/stat.h>   // ?
#include <fcntl.h>      // open()
#include <string.h>     // strlen()
#include <errno.h>      // errno
#include <signal.h>     // for signals

#define MAX_ARG_COUNT 2 // Includes program name
#define BUFFER_SIZE 1024

static void signal_handler(int signal_number);

void usage(const char *progname);
void socketLogger(bool runAsDaemon);

void receivePacket(int client_fd, int file_fd);
ssize_t sendFileToSocket(int client_fd, int file_fd);

bool caught_exit_signal = false;

//This function was partially generated using ChatGPT at https://chat.openai.com/ with prompts including "Write me a usage function that might describe how a linux utility is used".
void usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "A simple example socket utility. Appends data received on port 9000 to /var/temp/aesdsocketdata\n"
        "\n"
        "Options:\n"
        "  -d       Runs this application as a Daemon\n"
        "\n",
        progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char*argv[]){
    if(argc > MAX_ARG_COUNT){
        printf("Error. Too many arguments.\n");
        usage(argv[0]);
    }

    if(argc == 1){
        socketLogger(false);
    }

    if(argc == 2 && strcmp(argv[1], "-d") != 0){
        printf("Error. Unknown Argument.\n");
        usage(argv[0]);
    } else {
        socketLogger(true);
    }
}

void socketLogger(bool runAsDaemon){
    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        exit(EXIT_FAILURE);
    }
    if(sigaction(SIGTERM, &sa, NULL) == -1){
        exit(EXIT_FAILURE);
    }
    
    //open socket to port 9000
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
        exit(EXIT_FAILURE);
    }

    //bind socket
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, "9000", &hints, &res) != 0){
        //failed to get socket info
        exit(EXIT_FAILURE);
    }

    if(bind(socket_fd, res->ai_addr, res->ai_addrlen) == -1){
        //failed to bind
        close(socket_fd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    //if daemon, fork
    if(runAsDaemon){
        if(daemon(0,0) == -1){
            //daemon failed
            exit(EXIT_FAILURE);
        }
    }

    if(listen(socket_fd, 1) == -1){
        //failed to listen
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    //open /var/tmp/aesdsocketdata
    int log_fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_TRUNC, 0644);

    while(caught_exit_signal == false){
        //wait for connection
        struct sockaddr client_addr;
        socklen_t addr_size = sizeof(client_addr);
        printf("Waiting for connection...\n");
        int client_fd = accept(socket_fd, &client_addr, &addr_size);

        char host[NI_MAXHOST];
        if (getnameinfo(&client_addr, addr_size, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
            printf("Accepted connection from %s\n", host);
        }

        if(client_fd == -1){
            continue;
        }

        //log successful connection to syslog
        syslog(LOG_DEBUG, "Accepted connection from %s", host);

        //block to receive data and append it to /var/tmp/aesdsocketdata
        receivePacket(client_fd, log_fd);

        sendFileToSocket(client_fd, log_fd);
        syslog(LOG_DEBUG, "Closed Signal from %s", host);
    }

    syslog(LOG_DEBUG, "Caught Signal, exiting");
    printf("Caught Signal, exiting\n");
    close(socket_fd);
    close(log_fd);
    
    if(remove("/var/tmp/aesdsocketdata") != 0){
        //failed to delete temp file
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

static void signal_handler(int signal_number){
    if(signal_number == SIGINT || signal_number == SIGTERM){
        caught_exit_signal = true;
    }
}

//This function was fully generated using ChatGPT at https://chat.openai.com/ with prompts including "write me a function that takes in a file_fd, and a socket_fd, and writes the entire file to the socket".
ssize_t sendFileToSocket(int socket_fd, int file_fd){
    lseek(file_fd, 0, SEEK_SET);

    char buffer[BUFFER_SIZE];
    ssize_t total_bytes_written = 0;

    while (1) {
        ssize_t bytes_read = read(file_fd, buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("read");
            return -1;
        } else if (bytes_read == 0) {
            // End of file
            break;
        }

        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t n = write(socket_fd, buffer + bytes_written, bytes_read - bytes_written);
            if (n < 0) {
                if (errno == EINTR) continue; // Interrupted, retry
                perror("write");
                return -1;
            }
            bytes_written += n;
        }

        total_bytes_written += bytes_written;
    }

    return total_bytes_written;
}

void receivePacket(int client_fd, int file_fd){
    char charRead = '\0';
    int bytesRead = -1;
    while (bytesRead != 0){
        bytesRead = recv(client_fd, &charRead, 1, 0);

        if(bytesRead == 1){
            write (file_fd, &charRead, 1);
        }

        if(charRead == '\n'){
            return;
        }
    }

    
}