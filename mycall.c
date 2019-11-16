#define _GNU_SOUrce
#include<unistd.h>
#include<sys/syscall.h>
int main(){
	syscall(332);
	return 0;
}