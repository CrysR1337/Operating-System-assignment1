
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>  
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

typedef struct Buffer
{
	int *ptr;
	int a;
	int counter;
	int current;
	sem_t *empty;
	sem_t *full;
	pthread_mutex_t mutex;
}Buffer; //仓库结构体

void *produce(Buffer *buffer)
{
    pid_t pid = (int)syscall(__NR_getpid); //系统调用获取进程id
	pid_t tid = (int)syscall(__NR_gettid); //系统调用获取线程id
	double y; //负指数分布的f值
	while(1)
	{
		y = ((double)rand()/RAND_MAX); //生成随机概率
		double a = buffer->a; //负指数分布的lambda值
		usleep(1000000*(log(y/a)/-a)); //计算出控制时间
		pthread_mutex_lock(&buffer->mutex);
		sem_wait(buffer->empty); //空位-1
		buffer->ptr[buffer->current] = rand();	
		buffer->counter += 1;
		printf("[pid: %d] [tid: %d] [value: %15d]\n",pid,tid,buffer->ptr[buffer->current++]);
		buffer->current %= 20;
		sem_post(buffer->full); //数量+1
		pthread_mutex_unlock(&buffer->mutex);
	}
}

int main(int argc, char **argv)
{
	sem_unlink("empty");
	sem_unlink("full");
	sem_t *empty;
	sem_t *full;
	empty = sem_open("empty", O_CREAT, 0666, 20); //初始化empty信号量
	full = sem_open("full", O_CREAT, 0666, 0); //初始化full信号量
	const int BUFFER_SIZE = sizeof(int) * 20;
	const char *name = "SharedMemory";

	int shm_fd;
	void *ptr;
	/* create the shared memory segment */
	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666); //初始化共享内存 可读取可创建
	/* configure the size of the shared memory segment */
	ftruncate(shm_fd,BUFFER_SIZE);
	/* now map the shared memory segment in the address space of the process */
	ptr = mmap(0,BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
		printf("Map failed\n");
		return -1;
	}
	Buffer *buffer = (Buffer*)malloc(sizeof(Buffer)); //初始化仓库
	buffer->ptr = (int*) ptr;
	buffer->a = atoi(argv[1]);	
    buffer->current = 0;
	buffer->counter = 0;
	buffer->empty = empty;
	buffer->full = full;

	pthread_t pro[3]; //3个线程的生产者
	pthread_attr_t attr[3];

	int resp;
	int i;
	for (i = 0; i < 3; i++){
        pthread_attr_init(&attr[i]);
		resp = pthread_create(&pro[i],&attr[i], (void *)produce, buffer);
		if(resp != 0){
			printf("Producer%d creation failed\n",i);
			exit(1);
		}
	}

	for (i = 0; i < 3; i++){
		pthread_join(pro[i],NULL);
	}

	printf("%d, %d, %d\n",*(buffer->ptr),*(buffer->ptr+1),*(buffer->ptr+2));
	return 0;
}