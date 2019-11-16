#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

pthread_mutex_t chopstick[6] ;//5根筷子作为mutex，创建6个为了让编号为12345

void *eat_think(void *arg)
{
	char phi = *(char *)arg;
    int wait_flag = 0;  //是否处在饥饿状态
	int left,right; //左右筷子的编号
	switch (phi){
		case 'A':
			left = 5;
			right = 1;
			break;
		case 'B':
			left = 1;
			right = 2;
			break;
		case 'C':
			left = 2;
			right = 3;
			break;
		case 'D':
			left = 3;
			right = 4;
			break;
		case 'E':
			left = 4;
			right = 5;
			break;
	}
    srand(time(NULL));
	for(;;){
        if(wait_flag != 1)
        {
            printf("Philosopher %c is thinking. (for %ds)\n",phi,rand()%5+1);
            sleep(rand()%5+1); //思考
            printf("Philosopher %c is hungry.\n",phi);
            wait_flag = 1;
        }
        else
        {
            if (pthread_mutex_trylock(&chopstick[right]) == EBUSY){ //拿起右手的筷子	
			    continue;
		    }
        }
		pthread_mutex_lock(&chopstick[left]); //拿起左手的筷子
		// printf("     Philosopher %c fetches chopstick %d\n", phi, left); //显示哪位哲学家拿了哪个筷子，监控运行状态
		if (pthread_mutex_trylock(&chopstick[right]) == EBUSY){ //拿起右手的筷子	
			pthread_mutex_unlock(&chopstick[left]); //如果右边筷子被拿走放下左手的筷子
            // printf("     Philosopher %c release chopstick %d\n", phi, left);
            printf("Philosopher %c is waiting.\n",phi);
			continue;
		}
		
	    // pthread_mutex_lock(&chopstick[right]); //拿起右手的筷子，把上一句if注释掉，再把这一句的注释去掉，即出现死锁
		// printf("     Philosopher %c fetches chopstick %d\n", phi, right); //显示哪位哲学家拿了哪个筷子，监控运行状态
		printf("Philosopher %c is eating. (for %ds)\n",phi,rand()%5+1);
        wait_flag = 0;
		sleep(rand()%5+1); //吃饭时间固定
		pthread_mutex_unlock(&chopstick[left]); //放下左手的筷子
		// printf("     Philosopher %c release chopstick %d\n", phi, left); //放下左手的筷子，把上一句if注释掉，再把这一句的注释去掉，即出现死锁
		pthread_mutex_unlock(&chopstick[right]); //放下右手的筷子
		// printf("     Philosopher %c release chopstick %d\n", phi, right); //放下右手的筷子，把上一句if注释掉，再把这一句的注释去掉，即出现死锁
        printf("Philosopher %c is full.\n",phi);
		pthread_mutex_destroy(&chopstick[left]);
		pthread_mutex_destroy(&chopstick[right]);
	}
}
int main(){
	pthread_t A,B,C,D,E; //5个哲学家

	int i;
	for (i = 0; i < 5; i++)
		pthread_mutex_init(&chopstick[i],NULL);
	pthread_create(&A,NULL, eat_think, "A");
	pthread_create(&B,NULL, eat_think, "B");
	pthread_create(&C,NULL, eat_think, "C");
	pthread_create(&D,NULL, eat_think, "D");
	pthread_create(&E,NULL, eat_think, "E");

	pthread_join(A,NULL);
	pthread_join(B,NULL);
	pthread_join(C,NULL);
	pthread_join(D,NULL);
	pthread_join(E,NULL);
	return 0;
}