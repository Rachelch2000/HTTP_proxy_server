//Racheli Chuwer
//211374111
#include "threadpool.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

void remove_node(threadpool *pool);
int f(void* in);
#define USAGE "Usage: threadpool <pool-size> <max-number-of-jobs>\n"


// int main(int argc, char* const argv[])
// {
//    threadpool *pool=create_threadpool(atoi(argv[1]));
//    printf("%d\n", pool->qsize);
//    int arr[3];
//    for(int i=0; i<3; i++)
//    {
//        arr[i]=i;
//        dispatch(pool, f, &arr[i]);
//    }
//    destroy_threadpool(pool);
//    printf("Hello, World!\n");
//    return 0;
// }
/*
 * create_threadpool creates a fixed-sized thread
 * pool.  If the function succeeds, it returns a (non-NULL)
 */
threadpool* create_threadpool(int num_threads_in_pool){
	//check that amount of threads is in range
    if(num_threads_in_pool < 1 || num_threads_in_pool > MAXT_IN_POOL){
        printf(USAGE);
        return  NULL;
    }
	
	//initialize threadpool
    threadpool *new_threadpool = (threadpool*)malloc(sizeof(threadpool)); 
    if (new_threadpool == NULL){
        perror("error: in malloc\n");
        return NULL;
	}
	
	//setting strart values of thread
    new_threadpool->num_threads = num_threads_in_pool;
    new_threadpool->qsize = 0;
	new_threadpool->threads = (pthread_t*)malloc(num_threads_in_pool*sizeof(pthread_t));
    if(new_threadpool->threads == NULL){
        free(new_threadpool);
        perror("error: in malloc\n");
        return NULL;
    }
    new_threadpool->qhead = NULL;
    new_threadpool->qtail = NULL;
    pthread_mutex_init (&(new_threadpool->qlock), NULL);
    pthread_cond_init(&(new_threadpool->q_not_empty), NULL);
    pthread_cond_init(&(new_threadpool->q_empty), NULL);
    new_threadpool->shutdown = 0;
    new_threadpool->dont_accept = 0;

	//creating pthread's
    int t, r;
    for(t = 0; t < num_threads_in_pool; t++){
        r = pthread_create(&(new_threadpool->threads[t]), NULL, do_work, (void*)new_threadpool);
        if(r) return NULL;
    }
    return new_threadpool;
}
/*
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
*/
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg){
	//null chack
    if(from_me == NULL|| dispatch_to_here == NULL|| arg == NULL){
        printf("NULL arg\n");
        return;
	}
	
	//start destructor
    //--!critical section!--
    pthread_mutex_lock(&from_me->qlock);
    if (from_me-> dont_accept == 1) return; 
    pthread_mutex_unlock(&from_me->qlock);
	//--!end of critical section!--

    //new work
    work_t* new_work = (work_t*) malloc(sizeof(work_t));
    if(new_work == NULL) 
    {
        perror("error: in malloc\n");
        return;
    }
    new_work->routine = dispatch_to_here;
    new_work->arg = arg;
    new_work->next = NULL;

    //adding new_work to queue
    //--!critical section!--
    pthread_mutex_lock(&from_me->qlock);
    if(from_me->qsize == 0){ //queue is empty
        from_me->qhead = new_work;
        from_me->qtail = new_work;
    }
    else{
        from_me->qtail->next = new_work;
        new_work->next = NULL;
        from_me->qtail = new_work;
    }
    from_me->qsize++;
    pthread_cond_signal(&from_me->q_not_empty);
    pthread_mutex_unlock(&from_me->qlock);
	//--!end of critical section!--

}
/**
 * The work function of the thread
 */
void* do_work(void* p)
{
	//null check
    if(p == NULL){
        printf("NULL arg\n");
        return NULL;
	}
    threadpool *pool = (threadpool*)p; //casting

	//run till that queue is empty
    while (1)
    {
        //--!critical section!--
        pthread_mutex_lock(&(pool->qlock));
        if(pool->shutdown) //destructor start, queue is empty
        {
            pthread_mutex_unlock(&(pool->qlock)); //lock flag
            return NULL;
        }

        //queue is empty
        while(!pool->qsize)
        {
            pthread_cond_wait(&(pool->q_not_empty),&(pool->qlock)); //threads can sleep
            if(pool->shutdown) //destructor start, queue is empty
            {
                pthread_mutex_unlock(&(pool->qlock));
				//--!end of critical section!--
                return NULL;
            }
        }
        remove_node(pool);
		//--!end of critical section!--
    }
    return NULL;
}
/**
 * destroy_threadpool kills the threadpool, causing
 * all threads in it to commit suicide, and then
 * frees all the memory associated with the threadpool.
 */
void destroy_threadpool(threadpool* destroyme){
	//null check
    if(destroyme == NULL){
        printf("NULL arg\n");
        return;
    }
    destroyme->dont_accept = 1; //stop getting work to do

    //--!critical section!--
    pthread_mutex_lock(&destroyme->qlock);
    //chack if there is tasks that did not start, queue is empty
    while(destroyme->qsize){
        pthread_cond_wait(&(destroyme->q_empty), &(destroyme->qlock));

    }
    destroyme->shutdown =1; //queue is empty, wake up waiting thread's
    pthread_cond_broadcast(&destroyme->q_not_empty); //set q_not_empty flag
    pthread_mutex_unlock(&destroyme->qlock);
	//--!end of critical section!--

    //join thread's
    for(int i=0; i<destroyme->num_threads; i++)
        pthread_join(destroyme->threads[i], NULL);

    //free all
    pthread_mutex_destroy(&(destroyme->qlock));
    pthread_cond_destroy(&(destroyme->q_empty));
    pthread_cond_destroy(&(destroyme->q_not_empty));
    free(destroyme->threads);
    free(destroyme);
}
/*
*removing a node from queue
*/
void remove_node(threadpool* pool){
    work_t *work = (pool->qhead);
    pool->qsize--;
    
    if(pool->qsize == 0) //queue is empty
    {
        pool->qhead = NULL;
        pool->qtail = NULL;
        //destroy
        if(pool->dont_accept) pthread_cond_signal(&(pool->q_empty));
    }
    else //queue is not empty
    { 
		pool->qhead = pool->qhead->next;
	}
    pthread_mutex_unlock(&(pool->qlock));
	//--!end of critical section!--

    //do work
    if(work->routine(work->arg) < 0){
        printf("ERROR processing request\n");
        return;
    }
    free(work);
}
// int f(void* in)
// {
//    int index=*((int*)in);
//    for(int i=0; i<3; i++)
//    {
//        printf("%d\n", index);
//        sleep(1);
//    }
// }
