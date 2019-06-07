# 内核同步

## 内核控制路径

内核控制路径： 1. 系统调用 2. 中断

单CPU中，内核控制路径并发：
1. 路径A执行到中间放弃CPU，CPU执行另一个系统调用
2. 系统调用A执行时，响应中断控制路径D

单CPU原子性策略：
1. 不放弃CPU, 禁剥夺 preempt_count++
2. 关中断


多CPU系统(SMP), 每个核构成独立的调度系统：
1. cpu_i上的控制路径原子操作 并且 其他核上与i上路径冲突的其他路径不能前进： spin_lock， 用来保护全局变量。

### 每次上锁或者动计数器，系统性能就会下降。
1. 尽量用局部变量
2. 尽量不用同步手段


Example:
1. CPU_A: 100个ready进程
2. CPU_B: 1个ready进程
3. 从A移40个到B: 先把40个进程瓜子啊migration_queue(runqueue中的per_cpu)上,然后锁住A，把migration_queue指针告诉B


## spin_lock

slock: 1->开锁， <=0->关锁

确保操作实施过程中，对slock设置和检测一次完成，其间不允许其他cpu访问slock

## Read/Write spin lock

## SeqLock

利用奇偶性

前提是写操作的频率低

## RCU : Read Copy Update

多个读写无锁前进

场合两个

# 作业: 多生产者 多消费者无锁环形队列


