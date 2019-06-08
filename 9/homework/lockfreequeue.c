#include <string.h>
#include "lockfreequeue.h"

// linked queue implementation, suffering from ABA problem

linked_queue init_linked_queue(){
	linked_queue retq;
	retq.head = (linked_queue_node*)malloc(sizeof(linked_queue_node));
	retq.head->data = NULL;
	retq.head->next = retq.head;
	retq.tail = retq.head;
	retq.size = 0;
	return retq;
}


/*
* 逻辑: 获取当前队列记录的tail，如果tail->next不是NULL,说明在获取之后修改之前
* 别的线程已经修改了tail, 此时重新尝试。
*
* 语句B: 如果有一个线程执行完语句C成功之后但是在语句D执行之前idle了或者挂掉了，如果没
* 有语句B, 队列的tail只能被idle的线程更新，此时其他线程判定queue->tail->next都
* 不是NULL,那么所有其他线程都会死循环; 如果有语句B，其他线程就会依次把queue->tail更
* 新到真正的队尾。
* 
* 语句C: 如果current_tail->next不是NULL, 说明其他线程已经抢先入队了新结点，此时
* 重新尝试。
* 
* 语句A: 理论上如果没有语句A，线程运行到B和C的时候一样可以判断
* 出current_tail!=queue_tail(当current_tail!=queue_tail时,一定有另一个线程已经运行
* 过D, 此时current_tail一定在queue->tail之前，则current_tail->next一定不是NULL，
* 即由A的条件一定可以推出B的条件，那么如果没有A的话，在B的时候也会continue)。这里
* 语句A主要是为了提高性能，避免进入CAS锁总线，从而进行性能优化。
* 
* 语句D: 语句D不判断成功是因为即便不成功queue->tail也会被其他进程的语句B更新正确。
*/
void linkedq_push(linked_queue* queue, void* data){
	linked_queue_node *current_tail, *node;
	node = (linked_queue_node*)malloc(sizeof(linked_queue_node));
	node->data = data;
	node->next = NULL;

	while(true){
		current_tail = queue->tail;            
		
		if(current_tail != queue->tail){ // A
			continue;
		}

		if(current_tail->next != NULL){ // B
			CAS(&queue->tail, current_tail, current_tail->next);
			continue;
		}

		if(CAS(&current_tail->next, NULL, node)){ // C
			break;
		}
	};

	CAS(queue->tail, current_tail, current_tail->next); //D
}

/*
* 语句A: 判断当前队列是否是空。不能用queue->head == queue->tail 因为有可能
* 其他线程push了但是还没来得及更新queue->tail
* 
* 语句B: 执行到这步时，queue->head的next一定不是NULL, 如果queue->head==queue->tail,
* 那么queue->tail一定是落后了，为了防止所有push进程都idle的情况, 此时pop进程也会
* 帮助更新queue->tail
*
* 语句C: 获取当前队列头
*
* 语句D: 如果获取的队列头和当前队列头相同，那么把当前队列头向前移动一个，把下个节点
* 当作dummy节点，并把其data取出来，然后释放空间。
*/
void* linkedq_pop(linked_queue* queue){
	linked_queue_node *current_head, *current_tail;
	void* data;
	while(true){
		if(queue->head->next == NULL){ // A
			return NULL; //empty
		}
		
		current_tail = queue->tail;
		if(queue->head == queue->tail){ // B
			CAS(&queue->tail, current_tail, queue->tail->next);
			continue;
		}

		current_head = queue->head;  // C

		if(CAS(&queue->head, current_head, current_head->next)){ //D
			data = current_head->next->data;
			free(current_head);
			return data;
		}else{
			continue;
		}
	}
	assert(0);
	return NULL;
}

/*
* ABA Problem:
* 假设现在队列是(dummy)->(A)
* 1. T1 进程调用pop，并且在D语句之前被T2抢占, 此时队列是 (dummy(*))->(A)
* 2. T2 上台运行，同样运行了一个pop并且成功，T2结束运行，(dummy(A))
* 3. T3 进程接着上台运行，T3 push了一个结点到队尾, 由于共享内存，新push的节点
* 的地址可能和T2 中free的节点的地址相同，即(dummy(A))->(C)
*/




