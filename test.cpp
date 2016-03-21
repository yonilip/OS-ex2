/*
 * test1.cpp
 *
 *	test suspends and resume
 *
 *  Created on: Apr 6, 2015
 *      Author: roigreenberg
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <deque>
#include <list>
#include <assert.h>
#include "uthreads.h"
//#include "libuthread.a"
#include <iostream>

using namespace std;


void f (void)
{
    int i = 1;
    int j = 0;
    while(1)
    {
        if (i == uthread_get_quantums(uthread_get_tid()))
        {
            cout << "f" << "  q:  " << i << endl;
            if (i == 3 && j == 0)
            {
                j++;
                uthread_sleep(-2);
            }
            if (i == 6 && j == 1)
            {
                j++;
            }
            if (i == 8 && j == 2)
            {
                j++;
                cout << "          **f end**" << endl;
                uthread_terminate(uthread_get_tid());
                return;
            }
            i++;
        }
    }
}

void g (void)
{
    int i = 1;
    int j = 0;
    while(1)
    {
        if (i == uthread_get_quantums(uthread_get_tid()))
        {
            cout << "g" << "  q:  " << i << endl;
            if (i == 11 && j == 0)
            {
                j++;
                cout << "          **g end**" << endl;
                uthread_terminate(uthread_get_tid());
                return;
            }
            if (i == 3 && j == 0)
            {

                uthread_sleep(3);
            }

            i++;
        }
    }
}


int main(void)
{
    if (uthread_init(100) == -1)
    {
        return 0;
    }

    int i = 1;
    int j = 0;

    while(1)
    {
        //int a = uthread_get_quantums(uthread_get_tid());
        //cout<<"quantums of thread number " << uthread_get_tid()<<" is " <<a<<std::endl;
        if (i == uthread_get_quantums(uthread_get_tid()))
        {
            cout << "m" << "  q:  " << i << endl;
            if (i == 3 && j == 0)
            {
                j++;
                cout << "          spawn f at (1) " << uthread_spawn(f) << endl;
                cout << "          spawn g at (2) " << uthread_spawn(g) << endl;
            }

            if (i == 6 && j == 1)
            {
                j++;
            }
            if (i == 9 && j == 2)
            {
                j++;
            }
            if (i == 13 && j == 3)
            {
                j++;
            }
            if (i == 17 && j == 4)
            {
                j++;
            }
            if (i == 20 && j == 5)
            {
                j++;
                //cout << "i: " << i << endl;
                cout << "          ******end******" << endl;
                cout << "total quantums:  " << uthread_get_total_quantums() << endl;
                uthread_terminate(0);
                return 0;
            }
            i++;
        }
    }
    cout << "end" << endl;
    return 0;
}



