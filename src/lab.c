#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

/**
 * @brief Structure representing a queue (monitor)
 */
struct queue {
    void **buffer;          
    int capacity;            
    int size;                
    int front;               
    int rear;                
    bool is_shutdown_flag;   
    pthread_mutex_t lock;    
    pthread_cond_t not_full;
    pthread_cond_t not_empty; 
};

/**
 * @brief Initialize a new queue
 *
 * @param capacity the maximum capacity of the queue
 * @return A fully initialized queue
 */
queue_t queue_init(int capacity) {
    assert(capacity > 0);
    
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (q == NULL) {
        perror("Failed to allocate memory for queue");
        return NULL;
    }

    q->buffer = (void **)malloc(capacity * sizeof(void *));
    if (q->buffer == NULL) {
        perror("Failed to allocate memory for queue buffer");
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1;
    q->is_shutdown_flag = false;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(q->buffer);
        free(q);
        return NULL;
    }

    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_mutex_destroy(&q->lock);
        free(q->buffer);
        free(q);
        return NULL;
    }

    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->lock);
        free(q->buffer);
        free(q);
        return NULL;
    }

    return q;
}

/**
 * @brief Frees all memory and related data signals all waiting threads.
 *
 * @param q a queue to free
 */
void queue_destroy(queue_t q) {
    if (q == NULL) {
        return;
    }

    // Ensures queue is shut down and all threads are notified
    queue_shutdown(q);

    // Clean up synchronization primitives
    pthread_mutex_lock(&q->lock);
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);

    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->lock);

    free(q->buffer);
    free(q);
}

/**
 * @brief Adds an element to the back of the queue
 *
 * @param q the queue
 * @param data the data to add
 */
void enqueue(queue_t q, void *data) {
    if (q == NULL) {
        return;
    }

    pthread_mutex_lock(&q->lock);

    // Waits until there is space in the queue or shutdown is initiated
    while (q->size == q->capacity && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    // Don't enqueue if the queue is shutting down
    if (q->is_shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    // Adds element to the queue
    q->rear = (q->rear + 1) % q->capacity;
    q->buffer[q->rear] = data;
    q->size++;

    // Signals that the queue is not empty
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

/**
 * @brief Removes the first element in the queue.
 *
 * @param q the queue
 * @return The removed element or NULL if queue is empty or shutdown
 */
void *dequeue(queue_t q) {
    if (q == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&q->lock);

    // Waits until there is an element in the queue or shutdown is initiated
    while (q->size == 0 && !q->is_shutdown_flag) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    // Returns NULL if the queue is empty
    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // Removes element from the queue
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    // Signals that the queue is not full
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return data;
}

/**
 * @brief Set the shutdown flag in the queue so all threads can
 * complete and exit properly
 *
 * @param q The queue
 */
void queue_shutdown(queue_t q) {
    if (q == NULL) {
        return;
    }

    pthread_mutex_lock(&q->lock);
    q->is_shutdown_flag = true;
    
    // Wakes up all waiting threads so they can check the shutdown flag
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

/**
 * @brief Returns true if the queue is empty
 *
 * @param q the queue
 * @return true if empty, false otherwise
 */
bool is_empty(queue_t q) {
    if (q == NULL) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);

    return empty;
}

/**
 * @brief Checks if the queue is in shutdown state
 *
 * @param q The queue
 * @return true if shutdown, false otherwise
 */
bool is_shutdown(queue_t q) {
    if (q == NULL) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool shutdown = q->is_shutdown_flag;
    pthread_mutex_unlock(&q->lock);

    return shutdown;
}