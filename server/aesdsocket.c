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
#include <sys/queue.h>  // for queue
#include <pthread.h>    // for threads
#include <time.h>       // for timer

#define MAX_ARG_COUNT 2 // Includes program name
#define BUFFER_SIZE 1024// Maximum buffer

static void signal_handler(int signal_number);

void usage(const char *progname);
void socketLogger(bool runAsDaemon);
int waitForConnection(int socket_fd);
void receivePacket(int client_fd, int file_fd);
ssize_t sendFileToSocket(int client_fd, int file_fd);
int bindPortToSocket();
void registerSignalHandler();
void Daemonize(bool beDaemon);
void* client_handler(void* args);
timer_t createTimer(int file_fd);
void timestamp_handler(union sigval sv);

typedef struct thread_args_t {
    int file_fd;
    int client_fd;
} thread_args_t;

typedef struct thread_item {
    pthread_t thread;
    thread_args_t thread_args;
    TAILQ_ENTRY(thread_item) entries;
} thread_item_t;
TAILQ_HEAD(thread_queue, thread_item);

bool caught_exit_signal = false;
int socket_fd = -1;

pthread_mutex_t log_lock;

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
        return 0;
    }

    if(argc == 2 && strcmp(argv[1], "-d") != 0){
        printf("Error. Unknown Argument.\n");
        usage(argv[0]);
    } else {
        socketLogger(true);
        return 0;
    }
}

void socketLogger(bool runAsDaemon){
    openlog("aesdsocket", LOG_PID, LOG_USER);
    registerSignalHandler();
    socket_fd = bindPortToSocket();

    Daemonize(runAsDaemon);

    if(listen(socket_fd, 1) == -1){
        perror("Failed to Listen");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    struct thread_queue queue;
    TAILQ_INIT(&queue);

    //open /var/tmp/aesdsocketdata
    int log_fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if(pthread_mutex_init(&log_lock, NULL) != 0){
        perror("Failed to Init Mutex");
        return;
    }

    timer_t timerid = createTimer(log_fd);

    while(caught_exit_signal == false){
        int client_fd = waitForConnection(socket_fd);

        if(client_fd == -1){
            break;
        }

        thread_item_t* threadPtr = malloc(sizeof(thread_item_t));
        if(!threadPtr){
            perror("Malloc Failed");
            break;
        }
        threadPtr->thread_args.file_fd = log_fd;
        threadPtr->thread_args.client_fd = client_fd;

        if(pthread_create(&(threadPtr->thread), NULL, client_handler, &(threadPtr->thread_args)) != 0){
            perror("Failed to create pthread");
            break;
        }

        TAILQ_INSERT_TAIL(&queue, threadPtr, entries);
    }

    syslog(LOG_DEBUG, "Caught Signal, exiting");
    printf("Caught Signal, exiting\n");

    timer_delete(timerid);

    while(!TAILQ_EMPTY(&queue)){
        thread_item_t* threadPtr = TAILQ_FIRST(&queue);
        pthread_join(threadPtr->thread, NULL);
        TAILQ_REMOVE(&queue, threadPtr, entries);
        free(threadPtr);
    }

    close(socket_fd);
    printf("Socket Closed\n");
    close(log_fd);

    if(remove("/var/tmp/aesdsocketdata") != 0){
        //failed to delete temp file
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

void* client_handler(void* args){
    thread_args_t* thread_args = (thread_args_t*) args;
    int log_fd = thread_args->file_fd;
    int client_fd = thread_args->client_fd;

    receivePacket(client_fd, log_fd);
    printf("Bytes sent: %lu\n", sendFileToSocket(client_fd, log_fd));
    close(client_fd);

    return NULL;
}

static void signal_handler(int signal_number){
    if(signal_number == SIGINT || signal_number == SIGTERM){
        caught_exit_signal = true;
        if(socket_fd != -1){
            shutdown(socket_fd, SHUT_RDWR);
        }
    }
}

//This function was fully generated using ChatGPT at https://chat.openai.com/ with prompts including "write me a function that takes in a file_fd, and a socket_fd, and writes the entire file to the socket".
ssize_t sendFileToSocket(int socket_fd, int file_fd){
    

    char buffer[BUFFER_SIZE];
    ssize_t total_bytes_written = 0;

    pthread_mutex_lock(&log_lock);
    off_t ExternalPosition = lseek(file_fd, 0, SEEK_CUR);
    lseek(file_fd, 0, SEEK_SET);
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
    lseek(file_fd, ExternalPosition, SEEK_SET);
    pthread_mutex_unlock(&log_lock);

    return total_bytes_written;
}

//This function was fully generated using ChatGPT at https://chat.openai.com/ with prompts including
//"Can you rewrite this function to use a dynamically allocated buffer? It should receive until it receives a newline character, and then it should write the buffer to file_fd and return:" and my old version.
void receivePacket(int client_fd, int file_fd) {
    size_t buf_size = BUFFER_SIZE;
    size_t len = 0;
    char *buffer = malloc(buf_size);
    if (!buffer) {
        return; // malloc failed
    }

    char ch;
    ssize_t bytesRead;

    while ((bytesRead = recv(client_fd, &ch, 1, 0)) > 0) {
        // Append to buffer, resizing if needed
        if (len >= buf_size) {
            size_t new_size = buf_size * 2;
            char *tmp = realloc(buffer, new_size);
            if (!tmp) {
                free(buffer);
                return; // realloc failed
            }
            buffer = tmp;
            buf_size = new_size;
        }

        buffer[len++] = ch;

        if (ch == '\n') {
            // Write buffer to file
            ssize_t total_written = 0;
            pthread_mutex_lock(&log_lock);
            while (total_written < len) {
                ssize_t written = write(file_fd, buffer + total_written, len - total_written);
                if (written <= 0) {
                    if (errno == EINTR) continue; // interrupted, retry
                    break; // write error
                }
                total_written += written;
            }
            fsync(file_fd);
            pthread_mutex_unlock(&log_lock);
            free(buffer);
            return;
        }
    }

    // If connection closed before newline, write whatever we have
    if (len > 0) {
        ssize_t total_written = 0;
        while (total_written < len) {
            ssize_t written = write(file_fd, buffer + total_written, len - total_written);
            if (written <= 0) {
                if (errno == EINTR) continue;
                break;
            }
            total_written += written;
        }
        fsync(file_fd);
    }

    free(buffer);
}

int bindPortToSocket(){
    printf("Starting Bind\n");
    //open socket to port 9000
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
        perror("Failed to get socket_fd");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //bind socket
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, "9000", &hints, &res) != 0){
        perror("Failed to get socket info");
        exit(EXIT_FAILURE);
    }

    if(bind(socket_fd, res->ai_addr, res->ai_addrlen) == -1){
        perror("Failed to bind port");
        close(socket_fd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    printf("Bind Completed\n");
    return socket_fd;
}

int waitForConnection(int socket_fd){
    while(!caught_exit_signal){
        struct sockaddr_storage client_addr;   // <-- bigger, safe for all families
        socklen_t addr_size = sizeof(client_addr);

        printf("Waiting for connection...\n");
        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_size);
        if(client_fd == -1){
            if(errno == EINTR){
                if(caught_exit_signal){
                    return -1;
                }
                continue;
            } else if(errno == EBADF){
                break;
            }
            perror("accept");
            continue;
        }

        char host[NI_MAXHOST];
        if(getnameinfo((struct sockaddr*)&client_addr, addr_size,
                       host, sizeof(host),
                       NULL, 0,
                       NI_NUMERICHOST) == 0) {
            printf("Accepted connection from %s\n", host);
        }

        syslog(LOG_DEBUG, "Accepted connection from %s", host);
        syslog(LOG_DEBUG, "Closed Signal from %s", host);

        return client_fd;
    }
    return -1;
}

void registerSignalHandler(){
    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        exit(EXIT_FAILURE);
    }
    if(sigaction(SIGTERM, &sa, NULL) == -1){
        exit(EXIT_FAILURE);
    }
}

void Daemonize(bool beDaemon){
    if(beDaemon){
        if(daemon(0,0) == -1){
            //daemon failed
            exit(EXIT_FAILURE);
        }
    }
}
//This function was mostly generated using ChatGPT at https://chat.openai.com/ with prompts including "can you show me an example of a recurring timer (10sec)".
timer_t createTimer(int file_fd){
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timestamp_handler;
    /* store fd as an integer inside the pointer-sized field */
    sev.sigev_value.sival_ptr = (void*)(intptr_t)file_fd;
    sev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("Failed to create timer");
        exit(EXIT_FAILURE);
    }

    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 10;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1){
        perror("Failed to start timer");
        exit(EXIT_FAILURE);
    }

    return timerid;
}

//This function was partially generated using ChatGPT at https://chat.openai.com/ with prompts including "write me a function that writes wall time in RFC2822 format to a file"
void timestamp_handler(union sigval sv){ 
    printf("Timer Fired!\n"); 
    int log_fd = (int)(intptr_t)sv.sival_ptr;
    time_t rawtime; 
    struct tm *timeinfo; 
    char buffer[500]; 
    char timestamp_line[700]; 

    // Get current time 
    time(&rawtime); 
    timeinfo = localtime(&rawtime); 

    // Format RFC 2822 timestamp 
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %z", timeinfo); 

    // Build final string "timestamp:time\n" 
    snprintf(timestamp_line, sizeof(timestamp_line), "timestamp:%s\n", buffer); 

    // Write string 
    pthread_mutex_lock(&log_lock); 
    ssize_t len = strlen(timestamp_line); 
    if (write(log_fd, timestamp_line, len) != len) { 
        perror("timestamp write fail"); 
        close(log_fd); 
        exit(EXIT_FAILURE); 
    } 
    pthread_mutex_unlock(&log_lock);
}