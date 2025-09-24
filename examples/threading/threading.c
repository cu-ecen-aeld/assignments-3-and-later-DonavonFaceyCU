#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //

    if(thread_param == NULL){
        ERROR_LOG("Started Thread with nullptr to parameters");
        pthread_exit(thread_param);
    }
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    //wait before locking mutex
    usleep(thread_func_args->wait_to_obtain_ms);

    //lock mutex
    pthread_mutex_lock(thread_func_args->mutex);

    //wait after locking mutex
    usleep(thread_func_args->wait_to_release_ms);

    //unlock mutex
    pthread_mutex_unlock(thread_func_args->mutex);
    
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    pthread_t thread_id;

    struct thread_data* thread_param = (struct thread_data*) malloc(sizeof(struct thread_data));
    if(thread_param == NULL){
        return false;
    }

    thread_param->thread_complete_success = false;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->mutex = mutex;

    int ret = pthread_create(&thread_id, NULL, threadfunc, thread_param);

    //thread is created
    if(ret == 0){
        *thread = thread_id;
        return true;
    }

    //cleanup if thread was failed to be created
    free(thread_param);
    return false;
}

