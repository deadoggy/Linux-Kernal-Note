# ifndef LOCKFREEQUEUE_H_
# define LOCKFREEQUEUE_H_

#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<malloc.h>
#include<assert.h>

#define CAS(a_ptr, a_oldVal, a_newVal) __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

typedef struct _linked_queue_node {
    void* data;
    struct _linked_queue_node* next;
} linked_queue_node;

typedef struct _linked_queue {
    linked_queue_node* head;
    linked_queue_node* tail;
} linked_queue;

linked_queue* init_linked_queue();
void linkedq_push(linked_queue* queue, void* data);
void* linkedq_pop(linked_queue* queue);


typedef struct _array_queue_node {
    void* data;
} array_queue_node ;

typedef struct _array_queue {
    size_t capacity;
    size_t read_idx;
    size_t write_idx;
    size_t max_read_idx;
    array_queue_node* q;
} array_queue;

array_queue* init_array_queue(size_t capacity);
bool arrayq_push(array_queue* queue, void* data);
void* arrayq_pop(array_queue* queue);

# endif
