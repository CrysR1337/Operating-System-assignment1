# **操作系统大作业1**

陈政培 17363011 智能科学与技术 智科1班

------

> 此为GitHub便捷预览README文件，为无图版本的报告，以防报告出现错误，请以[陈政培 17363011 智能科学与技术 操作系统大作业1.pdf](https://github.com/sysu17363011/os-assignment1/blob/master/陈政培 17363011 智能科学与技术 操作系统大作业1.pdf)为准
>

## **1.**  **哲学家就餐问题**

### 原理和具体实现

​	参考教材，使用`pthreads_cond_t`条件变量和`pthread_mutex_t`互斥量，达到加锁的功能

​	将5根筷子作为作为5个互斥量，记录筷子的使用情况

```c
pthread_mutex_t chopstick[6] ;

int i;
for (i = 0; i < 5; i++)
	pthread_mutex_init(&chopstick[i],NULL);
```

​	5个哲学家对应五个线程，每个线程拥有两个互斥量的掌控权

### 死锁解决方案

​	当一个哲学家感到饥饿，将首先拿起左手的筷子，如果此时右手的筷子并不处于空闲状态，则将左手筷子放回桌子上并进入等待状态。等待状态中，持续等待右手筷子出现，当两只筷子都空闲时开始吃饭

```c
if (pthread_mutex_trylock(&chopstick[right]) == EBUSY){ //拿起右手的筷子	
	pthread_mutex_unlock(&chopstick[left]); //如果右边筷子被拿走放下左手的筷子 
	printf("Philosopher %c is waiting.\n",phi);
	continue;
}
```

避免死锁的核心就是这个`if`判断语句，如果注释掉，则可以观察到死锁状态

### 程序运行结果

哲学家有等待或者立即吃饭的状态，并且没有出现死锁



## **2.**  **生产者消费者问题**

### 原理和具体实现

​	结合教材提示，使用`empty`和`full`两个标准的计数信号量，并且通过`mutex`互斥量保护对缓冲区的插入和删除操作。并将仓库存放的数据`value`、信号量、互斥量以及工作计数、当前队列指针统一封装为`Buffer`结构体

#### 	Buffer结构体

```c
typedef struct Buffer
{
	int *ptr;
	int a;
	int counter;
	int current;
	sem_t *empty;
	sem_t *full;
	pthread_mutex_t mutex;
}Buffer;
```

#### 	生产者和消费者逻辑（伪代码逻辑）

```
produce {
    while( 1 ) {
        wait( space );  // 等待缓冲区有空闲位置
        wait( mutex ); 

        buffer.push( item, in );  // 加入新资源
        in = ( in + 1 ) % 20;
        
        signal( mutex ); 
        signal( items ); 
    }
}

consume {
    while( 1 ) {
        wait( items );  // 等待缓冲区有资源可以使用
        wait( mutex );  // 

        buffer.pop( out );  // 将资源取走
        out = ( out + 1 ) % 10;

        signal( mutex );
        signal( space );
    }
}
```

#### 	共享内存

​	通过系统自带的`shm`相关头文件，创建一个统一名称统一大小的内存空间

```c
	const int BUFFER_SIZE = sizeof(int) * 20;
	const char *name = "SharedMemory";

	int shm_fd;
	void *ptr;

	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

	ftruncate(shm_fd,BUFFER_SIZE);

	ptr = mmap(0,BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Map failed\n");
		return -1;
	}
```

#### 	踩坑

​	共享内存申请时，`BUFFER_SIZE`不能直接赋值20，申请的空间会在实际使用中出现数组越界的情况。应当赋值为`sizeof(int) * 20`以此按照`int`类型的大小申请内存

#### 	程序运行结果

​	在`main`函数中定义了传递参数`argv`，用来获得负指数分布的控制参数

```c
int main(int argc, char **argv)
```

​	负指数分布获得时间间隔

```c
y = ((double)rand()/RAND_MAX);
double a = buffer->a;
usleep(1000000*(log(y/a)/-a));
```

速度更快的一方，会在全空或全满时等待仓库的变化



## **3.**  **Linux**内核实验（Linux 4.0或以上）

### a	complete fair scheduler（CFS）内核代码阅读报告

#### Linux进程的基本结构

​	Linux内核通过进程描述符`task_struct`结构体来管理进程，这个结构体包含了进程所需要的所有信息。结构体定义在`include/linux/sched.h`中，由于`task_struct`算是`linux`内核代码中最复杂的结构体之一，成员非常多，代码量500行不止，所以就选取部分相关度高的代码进行解析

##### 进程状态

###### `state`

```c
/* -1 unrunnable, 0 runnable, >0 stopped: */
volatile long			state;
int				exit_state;
int				exit_code;
int				exit_signal;
```

​	state成员的可能取值有

```c
 #define TASK_RUNNING            0
 #define TASK_INTERRUPTIBLE      1
 #define TASK_UNINTERRUPTIBLE    2
 #define __TASK_STOPPED          4
 #define __TASK_TRACED           8

/* in tsk->exit_state */
 #define EXIT_DEAD               16
 #define EXIT_ZOMBIE             32
 #define EXIT_TRACE              (EXIT_ZOMBIE | EXIT_DEAD)

/* in tsk->state again */
 #define TASK_DEAD               64
 #define TASK_WAKEKILL           128    /** wake on signals that are deadly **/
 #define TASK_WAKING             256
 #define TASK_PARKED             512
 #define TASK_NOLOAD             1024
 #define TASK_STATE_MAX          2048

 /* Convenience macros for the sake of set_task_state */
#define TASK_KILLABLE           (TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED            (TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED             (TASK_WAKEKILL | __TASK_TRACED)
```

- 其中`TASK_RUNNING`、`TASK_INTERRUPTIBLE`、`TASK_UNINTERRUPTIBLE`、`__TASK_STOPPED`、`__TASK_TRACED`  是五个互斥的值，每个进程都必然处于其中一种

- 而`exit_state`、`exit_code`、`exit_signal`为`state`域另外的附加进程状态，一般当进程终止的时候，会出现这两种状态。包括`EXIT_ZOMBIE`、`EXIT_DEAD`

- 进程状态的切换过程和原理

- `TASK_KILLABLE`是除了`TASK_INTERRUPTIBLE`、`TASK_UNINTERRUPTIBLE`内核中实现的另外一种睡眠方式，原理和`TASK_UNINTERRUPTIBLE`类似但可以响应致命信号

###### 进程表示符 `PID`

```c
pid_t				pid;
pid_t				tgid;
```

- `linux`把不同的`pid`与系统中每个进程或轻量级线程关联，一个线程组所有线程与领头线程拥有相同的`pid`
- 所以在使用系统调用`SYS_getpid()`时，返回的是当前进程的`tgid`值

###### 其他结构

- 进程内核栈

  ```c
  void				*stack;
  ```

- 进程标记

  ```c
  unsigned int			flags;
  ```

- `Ptrace`系统调用

  ```c
  unsigned int			ptrace;
  ```

##### 进程调度

###### 优先级

```c
int				prio;
int				static_prio;
int				normal_prio;
unsigned int			rt_priority;
```

- `prio`保存动态优先级
- `static_prio`保存静态优先级，通过`nice`系统调用修改
- `rt_priority`保存实时优先级
- `normal_prio`由静态优先级和调度策略决定

###### 调度策略

```c
unsigned int			policy;

const struct sched_class	*sched_class;
struct sched_entity		se;
struct sched_rt_entity		rt;

cpumask_t			cpus_allowed;
```

- `policy`代表调度策略
- `sched_class`调度类，`se`为普通进程的调用实体，`rt`为实时进程的调用实体，作业b中系统调用显示的大部分信息都是在实体`se`内的
- `cpus_allowed`控制进程可以在什么处理器上运行

##### 结构体其他内容

###### 判断标志

```c
int exit_code, exit_signal;
int pdeath_signal;  /*  The signal sent when the parent dies  */
unsigned long jobctl;   /* JOBCTL_*, siglock protected */

/* Used for emulating ABI behavior of previous Linux versions */
unsigned int personality;

/* scheduler bits, serialized by scheduler locks */
unsigned sched_reset_on_fork:1;
unsigned sched_contributes_to_load:1;
unsigned sched_migrated:1;
unsigned :0; /* force alignment to the next boundary */

/* unserialized, strictly 'current' */
unsigned in_execve:1; /* bit to tell LSMs we're in execve */
unsigned in_iowait:1;
```

###### 时间

```c
cputime_t utime, stime, utimescaled, stimescaled;
cputime_t gtime;
struct prev_cputime prev_cputime;
#ifdef CONFIG_VIRT_CPU_ACCOUNTING_GEN
seqcount_t vtime_seqcount;
unsigned long long vtime_snap;
enum {
        /* Task is sleeping or running in a CPU with VTIME inactive */
        VTIME_INACTIVE = 0,
        /* Task runs in userspace in a CPU with VTIME active */
        VTIME_USER,
        /* Task runs in kernelspace in a CPU with VTIME active */
        VTIME_SYS,
} vtime_snap_whence;
#endif
unsigned long nvcsw, nivcsw; /* context switch counts */
u64 start_time;         /* monotonic time in nsec */
u64 real_start_time;    /* boot based time in nsec */
/* mm fault and swap info: this can arguably be seen as either mm-specific or thread-specific */
unsigned long min_flt, maj_flt;

struct task_cputime cputime_expires;
struct list_head cpu_timers[3];

/* process credentials */
const struct cred __rcu *real_cred; /* objective and real subjective task
                                 * credentials (COW) */
const struct cred __rcu *cred;  /* effective (overridable) subjective task
                                 * credentials (COW) */
char comm[TASK_COMM_LEN]; /* executable name excluding path
                             - access with [gs]et_task_comm (which lock
                               it with task_lock())
                             - initialized normally by setup_new_exec */
/* file system info */
struct nameidata *nameidata;
#ifdef CONFIG_SYSVIPC
/* ipc stuff */
struct sysv_sem sysvsem;
struct sysv_shm sysvshm;
#endif
#ifdef CONFIG_DETECT_HUNG_TASK
/* hung task detection */
unsigned long last_switch_count;
#endif
```

###### 信号处理

```c
struct signal_struct *signal;
struct sighand_struct *sighand;
1583 
sigset_t blocked, real_blocked;
sigset_t saved_sigmask; /* restored if set_restore_sigmask() was used */
struct sigpending pending;
1587 
unsigned long sas_ss_sp;
size_t sas_ss_size;
```

#### CPU调度架构

##### CPU调度准则

​	从CPU的吞吐量、周转时间、等待时间、响应时间等角度让CPU使用率尽可能高。在分时系统中，依靠进程优先级、进程分类等，依据进程调度策略优化调度

##### 进程与调度相关数据结构

###### 相关结构体

除了进程基本结构，还有`sched_entity`结构体

```c
struct sched_entity {
    struct load_weight		load;
    unsigned long			runnable_weight;
    struct rb_node			run_node;
    struct list_head		group_node;
    unsigned int			on_rq;
    
    u64				exec_start;
    u64				sum_exec_runtime;
    u64				vruntime;
    u64				prev_sum_exec_runtime;
    
	u64				nr_migrations;
	struct sched_statistics		statistics;
}
```

- 其中`load`代表负载权重和重除
- `run_node`表示红黑树上的节点
- `on_rq`表示是否在调度队列上
- `exec_start`、`sum_exec_runtime`、`vruntime`、`prev_sum_exec_runtime`则是有关调度的开始时间、执行总时间、虚拟运行时间、上次调度的执行总时间

```c
struct load_weight {
	unsigned long			weight;
	u32				inv_weight;
};
```

- 为`sched_entity`结构体中使用到的`load_weight`结构体

###### 相关调度类

```c
struct sched_class {
	const struct sched_class *next;
	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*yield_task)   (struct rq *rq);
	bool (*yield_to_task)(struct rq *rq, struct task_struct *p, bool preempt);
	void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);
	/*
	 * It is the responsibility of the pick_next_task() method that will
	 * return the next task to call put_prev_task() on the @prev task or
	 * something equivalent.
	 *
	 * May return RETRY_TASK when it finds a higher prio class has runnable
	 * tasks.
	 */
	struct task_struct * (*pick_next_task)(struct rq *rq,
					       struct task_struct *prev,
					       struct rq_flags *rf);
	void (*put_prev_task)(struct rq *rq, struct task_struct *p);
#ifdef CONFIG_SMP
	int  (*select_task_rq)(struct task_struct *p, int task_cpu, int sd_flag, int flags);
	void (*migrate_task_rq)(struct task_struct *p, int new_cpu);
	void (*task_woken)(struct rq *this_rq, struct task_struct *task);
	void (*set_cpus_allowed)(struct task_struct *p,
				 const struct cpumask *newmask);
	void (*rq_online)(struct rq *rq);
	void (*rq_offline)(struct rq *rq);
#endif
	void (*set_curr_task)(struct rq *rq);
	void (*task_tick)(struct rq *rq, struct task_struct *p, int queued);
	void (*task_fork)(struct task_struct *p);
	void (*task_dead)(struct task_struct *p);

	extern const struct sched_class stop_sched_class;
	extern const struct sched_class dl_sched_class;
	extern const struct sched_class rt_sched_class;
	extern const struct sched_class fair_sched_class;
	extern const struct sched_class idle_sched_class;
}
```

`sched_class`类为`sched_entity`相关的功能实现，例如其中的CFS调度则使用的是`cfs_rq`结构体，在下一部分详细描述

##### 运行队列

```c
struct rq {
	unsigned long nr_running;

	#define CPU_LOAD_IDX_MAX 5
	unsigned long cpu_load[CPU_LOAD_IDX_MAX];

	struct load_weight load;

	struct cfs_rq cfs; 

	unsigned long nr_uninterruptible;

	struct task_struct *curr, *idle; 
	struct mm_struct *prev_mm; 

	u64 clock, prev_clock_raw;
	u64 tick_timestamp;

#ifdef CONFIG_SMP
	struct sched_domain *sd;

	/* For active balancing */
	int active_balance;
	int push_cpu;

	struct task_struct *migration_thread; 
	struct list_head migration_queue;
#endif
}
```

为整个调度框架的数据结构总入口，调度框架和实现函数只识别运行队列

###### 优先级处理

```c
#define MAX_USER_RT_PRIO	100
#define MAX_RT_PRIO		MAX_USER_RT_PRIO

#define MAX_PRIO		(MAX_RT_PRIO + 40)
#define DEFAULT_PRIO	(MAX_RT_PRIO + 20)

#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)
#define PRIO_TO_NICE(prio)	((prio) - DEFAULT_PRIO)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)

#define SCHED_LOAD_SHIFT	10
#define SCHED_LOAD_SCALE	(1L << SCHED_LOAD_SHIFT)
#define NICE_0_LOAD		SCHED_LOAD_SCALE
#define NICE_0_SHIFT	SCHED_LOAD_SHIFT
```

##### 调度初始化

```c
void __init sched_init(void)
{
	for_each_possible_cpu(i) {
		struct rt_prio_array *array;
		struct rq *rq;

		rq = cpu_rq(i);
		init_cfs_rq(&rq->cfs, rq);
        
	}

	set_load_weight(&init_task);

	open_softirq(SCHED_SOFTIRQ, run_rebalance_domains, NULL);

	init_idle(current, smp_processor_id());
	current->sched_class = &fair_sched_class;
}
```

内核在系统启动时初始化调度框架的数据结构，调用的就是`sched_init`来初始化结构体`rq`并初始化第一个进程

###### 周期调度器

```c
void scheduler_tick(void)
{
	int cpu = smp_processor_id();
	struct rq *rq = cpu_rq(cpu);
	struct task_struct *curr = rq->curr;

	if (curr != rq->idle) /* FIXME: needed? */
		curr->sched_class->task_tick(rq, curr);

#ifdef CONFIG_SMP

	trigger_load_balance(rq, cpu);
#endif
}
```

周期调度器有`scheduler_tick`实现信息的统计和判断

###### 其他

- CPU调度的其他内容包括主调度器
- 调度实体
- 调度标志
- 创建进程、唤醒进程的一些函数

#### CFS调度算法  

##### 原理

​	内核在实现`CFS`时，使用权重来描述进程应该分得多少CPU时间，因为`CFS`只负责非实时进程，所以权重与 nice 值密切相关。一般这个概念是，进程每降低一个 nice 值（优先级提升）， 则多获得 10% 的CPU时间, 每升高一个 nice 值（优先级降低）, 则放弃 10% 的CPU时间

​	对于`CFS`调度类，内核不仅使用权重来衡量进程在一定CPU时间周期分得的时间配额，还是使用权重来衡量进程运行的虚拟时间的快慢 ———— 权重越大，流失得越慢

##### `CFS`类的结构和算法

###### 周期调度

```c
static void task_tick_fair(struct rq *rq, struct task_struct *curr)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &curr->se;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);

		update_curr(cfs_rq);

		if (cfs_rq->nr_running > 1)
			check_preempt_tick(cfs_rq, curr);
	}
}

static void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = rq_of(cfs_rq)->clock;
	unsigned long delta_exec;
	unsigned long delta_exec_weighted;

	delta_exec = (unsigned long)(now - curr->exec_start);

	curr->sum_exec_runtime += delta_exec;
	delta_exec_weighted = delta_exec;
    
	if (unlikely(curr->load.weight != NICE_0_LOAD)) {
		delta_exec_weighted = calc_delta_fair(delta_exec_weighted,
							&curr->load);
	}
	curr->vruntime += delta_exec_weighted;

	curr->exec_start = now;
}

static void check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;

	ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime)
		resched_task(rq_of(cfs_rq)->curr);
}
```

 `CFS`类的 `task_tick_fair()` 方法利用`CFS`的分时是将降低当前运行进程对其他待调度进程造成的延迟为目标的调度算法，在一定的调度周期内，调度队列内所有的进程都被调度运行一次，即公平对待每个进程 

###### 再次排队的实现

```c
static void put_prev_task_fair(struct rq *rq, struct task_struct *prev)
{
	struct sched_entity *se = &prev->se;
	struct cfs_rq *cfs_rq;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);

		if (prev->se.on_rq) {

			update_curr(cfs_rq);

			__enqueue_entity(cfs_rq, &prev->se);
		}

		cfs_rq->curr = NULL;
	}
}
```

###### 选择最优进程使用CPU资源

```c
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev)
{
	const struct sched_class *class;
	struct task_struct *p;

	if (likely(rq->nr_running == rq->cfs.nr_running)) {
		p = fair_sched_class.pick_next_task(rq);
		if (likely(p))
			return p;
	}

	class = sched_class_highest;
	for ( ; ; ) {
		p = class->pick_next_task(rq);
		if (p)
			return p;
		class = class->next;
	}
}

static struct task_struct *pick_next_task_fair(struct rq *rq)
{
	struct cfs_rq *cfs_rq = &rq->cfs;
	struct sched_entity *se;

	if (unlikely(!cfs_rq->nr_running))
		return NULL;

	do {
		if (first_fair(cfs_rq)) {
			se = __pick_next_entity(cfs_rq);
			if (se->on_rq) {

				__dequeue_entity(cfs_rq, se);

				se->exec_start = rq_of(cfs_rq)->clock;

				cfs_rq->curr = se;

				se->prev_sum_exec_runtime = se->sum_exec_runtime;
			}
		}

		cfs_rq = group_cfs_rq(se);
	} while (cfs_rq);

	return task_of(se);
}
```

###### 其他

- 类比CPU调度，`CFS`也有其对应的创建、唤醒函数  `task_new_fair()`、 `enqueue_task_fair()` 

#### 作业问题解答

##### 进程优先级、nice值和权重的关系

​	内核在实现`CFS`时，使用权重来描述进程应该分得多少CPU时间，因为`CFS`只负责非实时进程，所以权重与 nice 值密切相关。一般这个概念是，进程每降低一个 nice 值（优先级提升）， 则多获得 10% 的CPU时间, 每升高一个 nice 值（优先级降低）, 则放弃 10% 的CPU时间

​	对于`CFS`调度类，内核不仅使用权重来衡量进程在一定CPU时间周期分得的时间配额，还是使用权重来衡量进程运行的虚拟时间的快慢 ———— 权重越大，流失得越慢

#####  `CFS`调度器中的`vruntime`的基本思想是什么？是如何计算的？何时得到更新？其中的`min_vruntime`有什么作用？

```c
static inline s64 entity_key(struct cfs_rq *cfs_rq, struct sched_entity *se) 
{
	return se-> se->vruntime- cfs_rq->min_vruntime;
}
```

######  	基本思想：

​	我们需要对每一个task添加一个线索字段用于追踪task的执行时间以确保完全加权公平这个字段就是 "virtual runtime"。以执行时间不快不慢的task的绝对时间为基本归一化 

###### `min_vruntime`的作用

​	虚拟运行时间，因为进程的优先级不一样加入了权值的因素。`min_vruntime`就是最小虚拟运行时间，其目的是为了创造一个相对的概念， 为了移除`cfs_rq`对`set_task_cpu()` 中定义的函数的依赖性，我们需要一个不变的标准对各个不同的参数，即用一个定量衡量每一个变量

### b	添加系统调用：用户层测试程序`mycall.c`

#### 实现步骤

1. ##### 环境安装

   系统已经自带有`gcc`编译器、`vim`编辑器和编译内核的`make`

   只需要额外安装`bison`、`flex`、`libssl-dev`、`libncurses5-dev`，这一串软件用来配置内核 

   ```
   apt-get install bison flex libssl-dev libncurses5-dev
   ```

2. ##### 修改系统调用表

   在目前修改的`linux-source-4.10.0`目录下

   ```
   vim arch/x86/entry/syscalls/syscall_64.tbl
   ```

   在文件末尾分配自己的系统调用号

   ```
   332		64		crysr		sys_crysr
   ```

3. ##### 申明系统调用服务例程原型

   ```
   vim include/linux/syscalls.h
   ```

   ```c
   asmlinkage long sys_crysr(void);
   ```

4. ##### 实现系统调用服务

   ```
   vim kernel/sys.c
   ```

   此处由于工程量过大在`vim`编辑太不方便，所以使用了vs code连接虚拟机，并对`sys.c`进行修改

   但是由于vs code并没有权限直接修改系统文件，所以需要在虚拟机内修改`sys.c`的权限

   ```
   sudo chown -R osc:osc ./sys.c
   ```

   然后在`sys.c`末尾添加服务函数

   ```c
   SYSCALL_DEFINE0(crysr){
   	printk("sys_call(crysr) success:\n");
   	printk("se.exec_start			:	%llu\n",current->se.exec_start);
   	printk("se.vruntime                     :	%llu\n",current->se.vruntime);
   	printk("se.nr_migrations                :	%llu\n",current->se.nr_migrations);
   	printk("nr_switchs			:	%lu\n",current->nvcsw+current->nivcsw);
   	printk("nr_voluntary_switches		:	%lu\n",current->nvcsw);
   	printk("nr_involuntary_switches 	:	%lu\n",current->nivcsw);
   	printk("se.load.weight			:	%lu\n",current->se.load.weight);
   	printk("se.avg.load_sum			:	%llu\n",current->se.avg.load_sum);
   	printk("se.avg.util_sum			:	%u\n",current->se.avg.util_sum);
   	printk("se.avg.load_avg			:	%lu\n",current->se.avg.load_avg);
   	printk("se.avg.util_avg			:	%lu\n",current->se.avg.util_avg);
   	printk("se.last_update_time		:	%llu\n",current->se.avg.last_update_time);
   	return 0;
   }
   ```

   ###### 相关原理：

   - current指针指向的是`include/linux/sched.h`中的`task_struct`结构体，而结构体内部包含有需要打印的信息

   - 其中只有`nr_voluntary_switches`和`nr_involuntary_switches`所对应的信息，以及他们的加和`nr_switchs`是在结构体下的
   - 其余的信息都在`task_struct`结构体内部的`sched_entity`结构体`se`中

5. ##### 重新编译内核

   清楚残留的`.config`和`.o`文件，并配置内核

   ```
   make mrproper
   make menuconfig
   ```

   保存配置信息后编译内核

   ###### 踩坑：

   - 重新编译内核需要足够的硬盘存储空间和足够的内存空间，而一开始预设的空间是远远不够的，会在编译中途出现报错
   - 在`VirtualBox`中将虚拟机内存分配到`2048MB`以上
   - 将虚拟机虚拟介质分配到`30GB`以上，并通过`gparted-live-1.0.0-5-amd64.iso`磁盘控制管理工具，分配这些存储空间

   编译内核、编译模块、安装模块、安装内核

   ```
   make -j2
   make modules
   make modules_install
   make install
   ```

   其中`j2`和`j4`可以加快编译但是也同样会消耗更多的资源

6. ##### 重启

   ```
   reboot
   ```

7. ##### 编写用户层程序调用自定义的系统调用

   新建`mycall.c`文件，调用`syscall(332)`，编译运行

#### 结果展示

1. ##### 内核版本信息

2. ##### 其他信息（`screenfetch`命令获取）

3. **进程的调用信息**（`dmesg`命令获取）



## 参考文献

1.  https://blog.csdn.net/ruglcc/article/details/7814546/  **Makefile经典教程(掌握这些足够)** 
2.  https://blog.csdn.net/sinat_35297665/article/details/78467725  Linux经典问题—五哲学家就餐问题
3.  https://wenku.baidu.com/view/de5426edc1c708a1284a4469.html  操作系统实验报告生产者消费者问题
4.  https://blog.csdn.net/yaozhiyi/article/details/7561759  OS: 生产者消费者问题(多进程+共享内存+信号量)
5.  https://blog.csdn.net/panxj856856/article/details/83014015  Linux内核源码解析 - CFS调度算法
6.  https://blog.csdn.net/qq_41357569/article/details/83017667  操作系统实验一：linux内核编译及添加系统调用
7.   https://code.woboq.org/linux/linux/  Linux内核代码在线查找
8.  https://blog.csdn.net/BigData_Mining/article/details/89014263  Ubuntu下vscode获得root权限
9.  https://blog.csdn.net/gatieme/article/details/51383272  Linux进程描述符task_struct结构体详解--Linux进程的管理与调度
10.   https://blog.csdn.net/CrazyHeroZK/article/details/84586537  Linux内核调度框架和CFS调度算法
11.   https://blog.csdn.net/u010173306/article/details/46646109  linux调度器_第三代cfs(2)_分解代码_vruntime和min_vruntime大概理解
12.   https://blog.csdn.net/armlinuxww/article/details/97242063  Linux CFS调度算法核心解析