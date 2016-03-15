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
	std::cout << "in f " << i << std::endl;

}

void g(void)
{
	int i = 0;
	/*while(1){
		++i;
		//printf("in f (%d)\n",i);
		std::cout << "in g " << i << std::endl;
	}*/
	std::cout << "in g " << i << std::endl;

}

int main()
{
    uthread_init(300000);
    uthread_spawn(f);
	uthread_spawn(g);


	while (1);



}