#include <signal.h>
#include <pthread.h>

#include "logit.h"
#include "threads.h"

#ifdef TERMUX
#define pthread_cancel(th) pthread_kill(th, SIGTERM)
#endif

threadpool_t *httpd_threads = NULL;

void threadpool_signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        logit("Caught signal %d, initiating shutdown...", signal);
        if (httpd_threads) {
            httpd_threadpool_clean(httpd_threads);
        }

        logit("Shutdown complete. Exiting.");
        exit(0);
    }

	return;
}

void threadpool_register_signal_handler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = threadpool_signal_handler;

    if (sigaction(SIGINT, &sa, NULL) != 0) {
		logit("sigaction() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) != 0) {
		logit("sigaction() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

	return;
}

/* Add a new task to the thread pool */
int httpd_threadpool_add_task(threadpool_t *pool, void (*function)(void *), void *arg)
{
    pthread_mutex_lock(&(pool->lock));

    /* Check if the pool is shutting down */
    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
        logit("Threadpool is shutting down. Cannot add new task.");
        return -1;
    }

    /* Check if the task queue is full */
    if (pool->tasks_in_queue >= pool->task_queue_size) {
        pthread_mutex_unlock(&(pool->lock));
        logit("Task queue is full. Cannot add new task.");
        return -1;
    }

    /* Create a new task */
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    if (new_task == NULL) {
		logit("malloc() failed: %s", strerror(errno));
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }

    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;

    /* Add the new task to the end of the task queue */
    if (pool->task_queue == NULL) {
        pool->task_queue = new_task;
    } else {
        task_t *last_task = pool->task_queue;

        while (last_task->next != NULL) {
            last_task = last_task->next;
        }

        last_task->next = new_task;
    }

    pool->tasks_in_queue++;

    logit("Task added to the queue. Waiting to be picked up. Tasks in queue: %d", pool->tasks_in_queue);

    pthread_cond_signal(&(pool->notify));  /* Signal a worker thread */
    pthread_mutex_unlock(&(pool->lock));   /* Unlock the mutex */

    return 0;
}

/* Retrieve and remove a task from the task queue */
task_t *threadpool_get_task(threadpool_t *pool)
{
    if (pool->tasks_in_queue == 0) {
        return NULL;
    }

    /* Get the task from the front of the queue */
    task_t *task = pool->task_queue;
    pool->task_queue = task->next;
    pool->tasks_in_queue--;

    logit("Task removed from the queue. Tasks left in queue: %d", pool->tasks_in_queue);

    return task;
}

/* Worker thread function to process tasks */
void *threadpool_worker(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        /* Wait for tasks to be available or for shutdown signal */
        while (pool->tasks_in_queue == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        /* If shutdown is signaled, exit the thread */
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        /* Get a task from the queue */
        task_t *task = threadpool_get_task(pool);
        pthread_mutex_unlock(&(pool->lock));

        if (task != NULL) {
            logit("Running task ...");
            task->function(task->arg);  /* Execute the task */
            logit("Task completed execution");
            free(task);  /* Free the memory allocated for the task */
        }
    }

    return NULL;
}

/* Initialize the thread pool */
threadpool_t *httpd_threadpool_init(int nthreads, int queue_size)
{

    threadpool_t *pool = NULL;

    threadpool_register_signal_handler();

    pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (pool == NULL) {
		logit("malloc() failed: %s", strerror(errno));
        return NULL;
    }

    pool->thread_count = nthreads;
    pool->task_queue_size = queue_size;
    pool->tasks_in_queue = 0;
    pool->shutdown = false;
    pool->task_queue = NULL;

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * nthreads);
    if (pool->threads == NULL) {
		logit("malloc() failed: %s", strerror(errno));
        pthread_mutex_destroy(&(pool->lock));  /* Clean up resources */
        pthread_cond_destroy(&(pool->notify));
        free(pool);
        return NULL;
    }

    /* Create worker threads */
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, pool) != 0) {
			logit("pthread_create() failed: %s", strerror(errno));
            for (int j = 0; j < i; j++) {
                pthread_cancel(pool->threads[j]);  /* Cancel any already created threads */
            }

            free(pool->threads);
            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));
            free(pool);
            return NULL;
        }

        logit("[INIT] thread %lu", (pool->threads[i]));
    }

    logit("Threadpool created with %d threads", nthreads);

    return pool;
}

/* Clean the thread pool and clean up resources */
void httpd_threadpool_clean(threadpool_t *pool)
{
    pthread_mutex_lock(&(pool->lock));

    pool->shutdown = true;

    pthread_cond_broadcast(&(pool->notify)); /* Wake up all worker threads */
    pthread_mutex_unlock(&(pool->lock));

    /* Wait for all threads to finish */
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
        logit("[FINI] thread %lu", (pool->threads[i]));
    }

    free(pool->threads);

    /* Free any remaining tasks in the queue */
    while (pool->task_queue != NULL) {
        task_t *task = pool->task_queue;
        pool->task_queue = task->next;
        free(task);
    }

    pthread_mutex_destroy(&(pool->lock));  /* Destroy the mutex */
    pthread_cond_destroy(&(pool->notify)); /* Destroy the condition variable */

    free(pool);

    return;
}

/* EOF */
