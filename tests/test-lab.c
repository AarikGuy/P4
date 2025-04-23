#include "harness/unity.h"
#include "../src/lab.h"
#include <pthread.h>


// NOTE: Due to the multi-threaded nature of this project. Unit testing for this
// project is limited. I have provided you with a command line tester in
// the file app/main.cp. Be aware that the examples below do not test the
// multi-threaded nature of the queue. You will need to use the command line
// tester to test the multi-threaded nature of your queue. Passing these tests
// does not mean your queue is correct. It just means that it can add and remove
// elements from the queue below the blocking threshold.


void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

// Test data
static int test_data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Thread function arguments
typedef struct {
    queue_t queue;
    int* data;
    int count;
} thread_args_t;

// Thread function prototypes
void* producer_thread(void* arg);
void* consumer_thread(void* arg);

void test_create_destroy(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    queue_destroy(q);
}

void test_queue_dequeue(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1;
    enqueue(q, &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    queue_destroy(q);
}

void test_queue_dequeue_multiple(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1;
    int data2 = 2;
    int data3 = 3;
    enqueue(q, &data);
    enqueue(q, &data2);
    enqueue(q, &data3);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data2);
    TEST_ASSERT_TRUE(dequeue(q) == &data3);
    queue_destroy(q);
}

void test_queue_dequeue_shutdown(void)
{
    queue_t q = queue_init(10);
    TEST_ASSERT_TRUE(q != NULL);
    int data = 1;
    int data2 = 2;
    int data3 = 3;
    enqueue(q, &data);
    enqueue(q, &data2);
    enqueue(q, &data3);
    TEST_ASSERT_TRUE(dequeue(q) == &data);
    TEST_ASSERT_TRUE(dequeue(q) == &data2);
    queue_shutdown(q);
    TEST_ASSERT_TRUE(dequeue(q) == &data3);
    TEST_ASSERT_TRUE(is_shutdown(q));
    TEST_ASSERT_TRUE(is_empty(q));
    queue_destroy(q);
}

// Test empty queue operations
void test_empty_queue(void)
{
    queue_t q = queue_init(5);
    TEST_ASSERT_TRUE(q != NULL);
    TEST_ASSERT_TRUE(is_empty(q));
    queue_destroy(q);
}

// Test full queue detection
void test_queue_full(void)
{
    queue_t q = queue_init(3);
    TEST_ASSERT_TRUE(q != NULL);
    
    // Fill the queue
    enqueue(q, &test_data[0]);
    enqueue(q, &test_data[1]);
    enqueue(q, &test_data[2]);
    
    // Verify contents
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[2]);
    TEST_ASSERT_TRUE(is_empty(q));
    
    queue_destroy(q);
}

// Test circular buffer behavior
void test_circular_buffer(void)
{
    queue_t q = queue_init(3);
    TEST_ASSERT_TRUE(q != NULL);
    
    // Fill the queue
    enqueue(q, &test_data[0]);
    enqueue(q, &test_data[1]);
    enqueue(q, &test_data[2]);
    
    // Remove 2 items
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);
    
    // Add more items (these should wrap around in the circular buffer)
    enqueue(q, &test_data[3]);
    enqueue(q, &test_data[4]);
    
    // Verify correct order
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[2]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[3]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[4]);
    
    queue_destroy(q);
}

// Test NULL handling in queue operations
void test_null_queue_handling(void)
{
    // These should not crash
    queue_destroy(NULL);
    enqueue(NULL, &test_data[0]);
    void* result = dequeue(NULL);
    TEST_ASSERT_TRUE(result == NULL);
    TEST_ASSERT_TRUE(is_empty(NULL));
    TEST_ASSERT_TRUE(is_shutdown(NULL));
    queue_shutdown(NULL);
}

// Thread functions for multi-threaded tests
void* producer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    for (int i = 0; i < args->count; i++) {
        enqueue(args->queue, &args->data[i]);
    }
    
    return NULL;
}

void* consumer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    int count = 0;
    
    while (count < args->count) {
        void* item = dequeue(args->queue);
        if (item != NULL) {
            count++;
        }
        
        // Exit if we've processed all items or the queue is shutdown and empty
        if ((count >= args->count) || (is_shutdown(args->queue) && is_empty(args->queue))) {
            break;
        }
    }
    
    return NULL;
}

// Basic multi-threaded test
void test_basic_multithreaded(void)
{
    queue_t q = queue_init(5);
    TEST_ASSERT_TRUE(q != NULL);
    
    pthread_t producer, consumer;
    thread_args_t producer_args = {q, test_data, 5};
    thread_args_t consumer_args = {q, NULL, 5};
    
    // Create threads
    pthread_create(&consumer, NULL, consumer_thread, &consumer_args);
    pthread_create(&producer, NULL, producer_thread, &producer_args);
    
    // Wait for threads to complete
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    
    // Verify queue is empty
    TEST_ASSERT_TRUE(is_empty(q));
    
    queue_destroy(q);
}

// Test with a very small queue to stress test blocking behavior
void test_small_queue(void)
{
    queue_t q = queue_init(1);
    TEST_ASSERT_TRUE(q != NULL);
    
    // Fill the queue
    enqueue(q, &test_data[0]);
    
    // Drain the queue
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[0]);
    TEST_ASSERT_TRUE(is_empty(q));
    
    // Fill again
    enqueue(q, &test_data[1]);
    TEST_ASSERT_TRUE(dequeue(q) == &test_data[1]);
    
    queue_destroy(q);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_queue_dequeue);
    RUN_TEST(test_queue_dequeue_multiple);
    RUN_TEST(test_queue_dequeue_shutdown);
    RUN_TEST(test_empty_queue);
    RUN_TEST(test_queue_full);
    RUN_TEST(test_circular_buffer);
    RUN_TEST(test_null_queue_handling);
    RUN_TEST(test_basic_multithreaded);
    RUN_TEST(test_small_queue);
    
    return UNITY_END();
}