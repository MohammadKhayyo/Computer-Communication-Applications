#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "threadpool.h"
threadpool* create_threadpool(int num_threads_in_pool){
    if(num_threads_in_pool > MAXT_IN_POOL|| num_threads_in_pool <= 0) return NULL;
    threadpool* threadPool=(threadpool*)calloc(1,sizeof(threadpool));
    if(!threadPool){
        printf("Allocation failure: \n");
        return NULL;
    }
    threadPool->qsize=threadPool->dont_accept=threadPool->shutdown=0;
    threadPool->num_threads=num_threads_in_pool;
    threadPool->qhead=threadPool->qtail= NULL;
    threadPool->threads=(pthread_t *) calloc(num_threads_in_pool,sizeof(pthread_t));
    if(!threadPool->threads){
        printf("Allocation failure: \n");
        return NULL;
    }
    int check;
    check=pthread_mutex_init(&(threadPool->qlock), NULL);
    if(check==-1){
        perror("error: pthread_mutex_init\n");
        free(threadPool->threads);
        free(threadPool);
        return NULL;
    }
    check=pthread_cond_init(&(threadPool->q_empty), NULL);
    if(check==-1){
        perror("error: pthread_cond_init\n");
        free(threadPool->threads);
        pthread_mutex_destroy(&threadPool->qlock);
        free(threadPool);
        return NULL;
    }
    check=pthread_cond_init(&(threadPool->q_not_empty), NULL);
    if(check==-1){
        perror("error: pthread_cond_init\n");
        free(threadPool->threads);
        pthread_mutex_destroy(&threadPool->qlock);
        pthread_cond_destroy(&threadPool->q_empty);
        free(threadPool);
        return NULL;
    }
    int rc;
    for (int num = 0; num < num_threads_in_pool; num++) {
        rc= pthread_create(&threadPool->threads[num], NULL, do_work, threadPool);
        if(rc!=0){
            printf("ERROR: \n");
            free(threadPool->threads);
            pthread_mutex_destroy(&threadPool->qlock);
            pthread_cond_destroy(&threadPool->q_empty);
            pthread_cond_destroy(&threadPool->q_not_empty);
            free(threadPool);
            return NULL;
        }
    }
    return threadPool;
}
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg){
    if(from_me->dont_accept==1) return;
    work_t* work_threadPool=(work_t*)calloc(1,sizeof(work_t));
    if(!work_threadPool){
        printf("Allocation failure: \n");
        free(from_me->threads);
        pthread_mutex_destroy(&from_me->qlock);
        pthread_cond_destroy(&from_me->q_empty);
        pthread_cond_destroy(&from_me->q_not_empty);
        free(from_me);
        return;
    }
    work_threadPool->arg=arg;
    work_threadPool->routine=dispatch_to_here;
    pthread_mutex_lock(&from_me->qlock);
    if(from_me->qhead==NULL) from_me->qhead=from_me->qtail=work_threadPool;
    else {
        from_me->qtail->next = work_threadPool;
        from_me->qtail=work_threadPool;
    }
    from_me->qsize++;
    pthread_cond_signal(&from_me->q_not_empty);
    pthread_mutex_unlock(&from_me->qlock);
}
void* do_work(void* p){
    threadpool* threadPool= (threadpool *) p;
    work_t* Tmp_For_process=NULL;
    for(;;){
        pthread_mutex_lock(&threadPool->qlock);
        if (threadPool->qsize == 0 && threadPool->shutdown ==0) {
            pthread_cond_wait(&threadPool->q_not_empty, &threadPool->qlock);
        }
        if(threadPool->qsize == 0 && threadPool->shutdown==1){
            pthread_mutex_unlock(&threadPool->qlock);
            return NULL;
        }
        if(threadPool->qsize!=0 && threadPool->shutdown==0){
            Tmp_For_process=threadPool->qhead;
            threadPool->qhead=Tmp_For_process->next;
            Tmp_For_process->next=NULL;
            threadPool->qsize--;
        }
        if(threadPool->qsize==0 && threadPool->dont_accept==1){
            pthread_cond_signal(&threadPool->q_empty);
        }
        pthread_mutex_unlock(&threadPool->qlock);
        if(Tmp_For_process!=NULL) {
            Tmp_For_process->routine(Tmp_For_process->arg);
            free(Tmp_For_process);
        }
    }
}
void destroy_threadpool(threadpool* destroyme){
    destroyme->dont_accept = 1;
    pthread_mutex_lock(&destroyme->qlock);
    if(destroyme->qsize != 0) {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }
    destroyme->shutdown=1;
    pthread_cond_broadcast(&destroyme->q_not_empty);
    pthread_mutex_unlock(&destroyme->qlock);
    void* status;
    int rc;
    for (int i = 0; i < destroyme->num_threads; i++) {
        rc=pthread_join((pthread_t) (destroyme->threads[i]), &status);
        if(rc!=0){
            printf("ERROR: \n");
            return;
        }
    }
    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_empty);
    free(destroyme->threads);
    pthread_mutex_unlock(&destroyme->qlock);
    pthread_mutex_destroy(&destroyme->qlock);
    free(destroyme);
}