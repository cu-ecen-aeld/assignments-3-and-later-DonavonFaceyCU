/*

File: writer.c
Author: Donavon Facey

*/

#include <syslog.h>     // syslog()
#include <stdio.h>      // printf()
#include <unistd.h>     // close()
#include <sys/types.h>  // ?
#include <sys/stat.h>   // ?
#include <fcntl.h>      // ?
#include <string.h>     // strlen()
#include <errno.h>      // errno


#define EXPECTED_ARG_COUNT 3 // Includes program name

int main(int argc, char*argv[]){
    openlog("writer.c", LOG_PID, LOG_USER);
    
    if(argc != EXPECTED_ARG_COUNT){
        printf("Error. Needs two arguments\n");
        syslog(LOG_ERR, "Error. Needs two arguments");
        return 1;
    }

    char* filePath = argv[1];
    char* stringToWrite = argv[2];

    if(!filePath){ //Need to check for valid filepath
        printf("Please enter a valid filepath\n");
        syslog(LOG_ERR, "Error. Invalid Filepath");
        return 1;
    }

    if(!stringToWrite || *stringToWrite == '\0'){ //Need to check for valid filepath, not super robust
        printf("Please enter a valid string to write\n");
        syslog(LOG_ERR, "Error. Invalid String");
        return 1;
    }

    //delete file if it already exists
    unlink(filePath);

    //create file
    int fileDescriptor;
    fileDescriptor = creat(filePath, 0644);

    //check for valid file to write to
    if(fileDescriptor == -1){
        printf("Error. Failed to create file\n");
        syslog(LOG_ERR, "Error. Failed to create file");
        return 1;
    }

    //printf("Writing %s to %s\n", stringToWrite, filePath);
    syslog(LOG_DEBUG, "Writing %s to %s", stringToWrite, filePath);

    //code for safe writing taken from Textbook, Linux System Programming Page 49
    int length = strlen(stringToWrite);
    char* currentChar = stringToWrite;
    ssize_t ret, nr;
    while (length != 0 && (ret = write (fileDescriptor, stringToWrite, length)) != 0) {
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            break;
        }
        length -= ret;
        currentChar += ret;
    }

    close(fileDescriptor);
    closelog();
    return 0;
}