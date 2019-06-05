# Linux 调度

## 就绪进程链表

1. 执行active队列中的进程 arrays[0]， 当active队列变空，交换arrarys[0] 和 [1]
2. active队列中，一定有非空链表，对手是CPU正在执行或者马上执行的进程
3. 若该队列中有不止一个进程，这些进程RoundRobin， 队首进程若连续运行，指定时间后放弃CPU，排在当前队列尾部, 队首进程时间片到了之后进过期队列

- TimeSlice: 大时间片
- Granularity: 小时间片 

