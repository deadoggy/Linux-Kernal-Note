#include <string.h>
#include "lockfreequeue.h"

// linked queue implementation, suffering from ABA problem

linked_queue* init_linked_queue(){
	linked_queue* retq = (linked_queue*)malloc(sizeof(linked_queue));
	retq->head = (linked_queue_node*)malloc(sizeof(linked_queue_node));
	retq->head->data = NULL;
	retq->head->next = NULL;
	retq->tail = retq->head;
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

	CAS(&queue->tail, current_tail, current_tail->next); //D
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
* 3. T3 进程接着上台运行，T3 push了一个结点到队尾, 由于共享内存所以(C)的地址
* 有可能等于(dummy(*)), (dummy(A))->(C)
* 4. T4 进程上台，调用pop和push并且成功，(dummy(C))->(D)
* 5. T1 进程上台，因为检测到(dummy(C))和(dummy(*))的地址是一样的，所以CAS中的
* 比较会成功，T1 会把(D)删除
*/



// array lock free queue implementation

array_queue* init_array_queue(size_t capacity){
	array_queue* retq = (array_queue*)malloc(sizeof(array_queue));
	retq->capacity = capacity;
	retq->read_idx = 0;
	retq->write_idx = 0;
	retq->max_read_idx = 0;
	retq->q = (array_queue_node*)malloc((capacity) * sizeof(array_queue_node));
	return retq;
}

int get_real_idx(array_queue q, size_t idx){
	return (idx % q.capacity);
}


/*
* 逻辑: 先从queue.write_idx处取号， 如果取到的号和当前queue.write_queue的
* 值一样，那么就将queue.write_idx+1(语句A)， 并且修改自己拿到的号对应位置
* 的值。如果发现取到的号和queue.write_idx的值不一样，那就重新取号。将取到
* 号的位置填上数据后，把max_read_idx+1,代表读者可以多读一个数据结点。
* 
* 语句A: 如果current_write_idx和queue.write_idx一样，就吧queue.write_idx+1,
* 代表把current_write_idx位置留给该线程，其他线程要从下一个位置开始push。
*
* 语句B: 这步不用做同步操作是因为其他线程只会从queue.write_idx开始写，而此时
* queue.write_idx比current_write_idx大，而queue.max_read_idx又小于等于
* current_write_idx,即读者和写者都不会访问current_write_idx的位置，该线程
* 可以慢慢写这结点的数据
* 
* 语句C: 当current_write_idx的数据填好后，更新queue.max_read_idx。这里while循环和
* CAS的目的是等在自己线程之前的线程更新完成后再更新max_read_idx的值，从而做到了顺序
* 写。
*/

bool arrayq_push(array_queue* queue, void* data){
	size_t current_read_idx, current_write_idx;
	while(true){
		current_read_idx = queue->read_idx;
		current_write_idx = queue->write_idx;

		if(get_real_idx(*queue, current_write_idx+1)==get_real_idx(*queue, current_read_idx)){
			return false; //full
		}
		
		if(!CAS(&queue->write_idx, current_write_idx, current_write_idx+1)){ // A
			continue;
		}
		queue->q[current_write_idx].data = data; // B

		while(!CAS(&queue->max_read_idx, current_write_idx, current_write_idx+1));// C

		return true;
	}
}


/*
* 语句A: 把current_read_idx处的数据取出来
* 
* 语句B: 检查current_read_idx的数据是否被其他线程读过，如果没有的话就把queue.read_idx+1
* 并且返回数据；如果其他线程读取过的话就重新尝试。
* 
* 注意，不能把A/B写成:
* 
* ...
* if(!CAS(&queue.read_idx, current_read_idx, current_read_idx+1)){ //C
* 	continue;
* }
* return queue.q[current_read_idx].data; //D
* ...

* 因为C/D之间时线程可能会被抢占， 其他线程如果读写队列并且套圈又回到current_read_idx
* 的位置并修改了current_read_idx的值之后，该线程又夺回了CPU，此时返回的值就和被抢占前
* 的不一样了。
*/
void* arrayq_pop(array_queue* queue){
	int current_read_idx;
	void* ret_data;
	while(true){
		current_read_idx = queue->read_idx;

		if(get_real_idx(*queue, current_read_idx)==get_real_idx(*queue, queue->max_read_idx)){
			return NULL;
		}
		
		ret_data = queue->q[current_read_idx].data; // A

		if(CAS(&queue->read_idx, current_read_idx, current_read_idx+1)){ //B
			return ret_data;
		}
	}
	assert(0);
	return NULL;
}

