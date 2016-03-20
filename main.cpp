//
// Created by danielle.kut on 3/9/16.
//


#include <sys/unistd.h>
#include <stdio.h>
#include <iostream>
//#include "Thread.h"
#include "uthreads.h"
#define SECOND 1000000

void f(void)
{
	int i = 0;
	/*while(1){
		++i;
		//printf("in f (%d)\n",i);
		std::cout << "in f " << i << std::endl;
	}*/
	while (1)
	{

	//std::cout << "in f " << i << std::endl;
}
	uthread_terminate(uthread_get_tid());
}

void g(void)
{
	int i = 0;
	/*while(1){
		++i;
		//printf("in f (%d)\n",i);
		std::cout << "in g " << i << std::endl;
	}*/
	while (1){
		//std::cout << "in g " << i << std	{
//::endl;

	}
	uthread_terminate(uthread_get_tid());
}

int main()
{
	uthread_init(3000000);
	uthread_spawn(f);
	uthread_spawn(g);
	uthread_block(1);
	uthread_terminate(2);




	/*while (1)
	{
		//std::cout << "in main" << std::endl;
		//getchar();
		*//*pause();*//*
	/*usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);
	usleep(SECOND);*/

	while(1);



}