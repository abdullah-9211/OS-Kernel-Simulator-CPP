#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <wait.h>
#include <time.h>
#include <fcntl.h>
#include <queue>
#include "process.h"

using namespace std;

vector<process> processes;
queue<process> newQueue;
queue<process> readyQueue;
queue<process> waitingQueue;
pthread_mutex_t queueMutex1, queueMutex2, queueMutex3, printMutex, preemptMutex, preemptMutex2;
pthread_t timer_t2, putinReady;
pthread_t* cpuThreads;
string* a;
double timer = 0;
int contextSwitchCount = 0;
double readyTime = 0;
int terminatedCount = 0;
int count = 0;
int procCount = 0;
int noOfCpus;
process* curr;
bool addToReady = true;
int timeSlice = 0;
ofstream out;

//=======================================================================================================================================================//
//=========================================================LOCK PROTECTED QUEUE OPERATIONS===============================================================//
//=======================================================================================================================================================//


void enqueue1(process p)
{
    pthread_mutex_lock(&queueMutex1);
    newQueue.push(p);
    pthread_mutex_unlock(&queueMutex1);
}

process dequeue1()
{
    pthread_mutex_lock(&queueMutex1);
    process p = newQueue.front();
    newQueue.pop();
    pthread_mutex_unlock(&queueMutex1);

    return p;
}


void enqueue2(process p)
{
    pthread_mutex_lock(&queueMutex2);
    readyQueue.push(p);
    pthread_mutex_unlock(&queueMutex2);
}

process dequeue2()
{
    pthread_mutex_lock(&queueMutex2);
    process p = readyQueue.front();
    readyQueue.pop();
    pthread_mutex_unlock(&queueMutex2);

    return p;
}


void enqueue3(process p)
{
    pthread_mutex_lock(&queueMutex3);
    waitingQueue.push(p);
    pthread_mutex_unlock(&queueMutex3);
}

process dequeue3()
{
    pthread_mutex_lock(&queueMutex3);
    process p = waitingQueue.front();
    waitingQueue.pop();
    pthread_mutex_unlock(&queueMutex3);

    return p;
}


//=======================================================================================================================================================//
//==============================================================TIMER====================================================================================//
//=======================================================================================================================================================//

void* timerStart(void* args)
{
    while (terminatedCount < procCount)
    {
        sleep(0.4);
        timer += 0.1;
        if (!readyQueue.empty())
            readyTime += 0.1;
    }
    pthread_exit(NULL);
}


//=======================================================================================================================================================//
//=======================================================READY QUEUE ACCORDING TO ARRIVAL TIME===========================================================//
//=======================================================================================================================================================//


void* initReadyQueue(void* args)
{
    while (terminatedCount < procCount)
    {
        if (newQueue.empty())
            break;
        else if (timer >= newQueue.front().arrivalTime && addToReady)
        {
            process p = dequeue1();
            enqueue2(p);
            contextSwitchCount += 1;
        }
    }
    pthread_exit(NULL);
}


//=======================================================================================================================================================//
//======================================================NECESSARY SCHEDULING FUNCTIONS===================================================================//
//=======================================================================================================================================================//


void terminate(process &p, int num)
{
    p.state = 0;
    a[num] = "(IDLE)";
    terminatedCount += 1;
}

void print_details(int state, string name)
{
    pthread_mutex_lock(&printMutex);
    if (state == 3)
    {
	    cout<<timer<<"\t"<<"0\t"<<"0\t"<<"1\t";
	    out<<timer<<"\t"<<"0\t"<<"0\t"<<"1\t";
        for (int i = 0; i < noOfCpus; i++)
        {
            cout << a[i] << "\t";
            out << a[i] << "\t";
        }
        cout << "\t<<" << name << "<<\n";
        out << "\t<<" << name << "<<\n";
    }
    else
    {
        cout<<timer<<"\t"<<"1\t"<<"0\t"<<"0\t";
        out<<timer<<"\t"<<"1\t"<<"0\t"<<"0\t";
        for (int i = 0; i < noOfCpus; i++)
        {
            cout << a[i] << "\t";
            out << a[i] << "\t";
        }
                    
        cout <<"\t"<<"<<\t<<\n";
        out <<"\t"<<"<<\t<<\n";
    }

    pthread_mutex_unlock(&printMutex);
}

void yield(process &p, int num)
{
	p.state=3;
    contextSwitchCount += 1;
	enqueue3(p);
    a[num] = "(IDLE)";
	double currentTime=timer;
	while(p.ioTime > 0) {
        p.ioTime -= 0.1;
        sleep(0.1);
        print_details(p.state, p.pName);
	}
	
	dequeue3();
	p.state=2;
    a[num] = p.pName;
    contextSwitchCount += 1;
}


//=======================================================================================================================================================//
//=========================================================FCFS SCHEDULING===============================================================================//
//=======================================================================================================================================================//


void* fcfs(void* args)
{
    int num = *((int*)args);
	while (terminatedCount < procCount)
	{
        sleep(0.1);
		if (!readyQueue.empty())
		{
            contextSwitchCount += 1;
		    bool flag = true;
		    process p = dequeue2();
            a[num] = p.pName;
            p.state = 2;
		    double currentTime = timer;
		    while ((timer - currentTime) <= p.cpuTime) 
		    {
                print_details(p.state, p.pName);
                sleep(0.1);
		    	if(p.ioTime>0 && (timer - currentTime)>=p.cpuTime/p.ioTime && flag)
		    	{
                    p.cpuTime -= (timer - currentTime);
		    		yield(p, num);
		    		flag = false;
                    currentTime = timer;
		    	}
		    }
		    terminate(p, num);
		}
	}

	pthread_exit(NULL);
}

//=======================================================================================================================================================//
//=========================================================PRIORITY SCHEDULING===========================================================================//
//=======================================================================================================================================================//

void preempt(int num)
{
    pthread_mutex_lock(&preemptMutex);

    addToReady = false;

    vector<process> temp;
    while (!readyQueue.empty())
    {
        temp.push_back(dequeue2());
    }

    process temp_p;
    for (int i = 0;  i < temp.size(); i++)
    {
        for (int j = 0; j < temp.size()-i; j++)
        {
            if (temp[j].priority < temp[j+1].priority)
            {
                temp_p = temp[j];
                temp[j] = temp[j+1];
                temp[j+1] = temp_p;
            }
        }
    }

    if (temp[0].priority > curr[num].priority)
    {
        temp_p = curr[num];
        curr[num] = temp[0];
        temp[0] = temp_p;
        a[num] = curr[num].pName;
        contextSwitchCount += 1;
    }


    for (int i = 0; i < temp.size(); i++)
    {
        enqueue2(temp[i]);
    }

    addToReady = true;

    pthread_mutex_unlock(&preemptMutex);
}

void* priority(void* args)
{
    int num = *((int*)args);
	while (terminatedCount < procCount)
	{
        sleep(0.1);
		if (!readyQueue.empty())
		{
            contextSwitchCount += 1;
		    bool flag = true;
		    curr[num] = dequeue2();
            a[num] = curr[num].pName;
            curr[num].state = 2;
		    double currentTime = timer;
		    while (curr[num].cpuTime > 0) 
		    {
                curr[num].cpuTime -= 0.1;
                print_details(curr[num].state, curr[num].pName);
                sleep(0.1);
                if (readyQueue.size() > 1)
                    preempt(num);
		    	if(curr[num].ioTime>0 && (timer - currentTime)>=curr[num].cpuTime/curr[num].ioTime && flag)
		    	{
                    curr[num].cpuTime -= (timer - currentTime);
		    		yield(curr[num], num);
		    		flag = false;
                    currentTime = timer;
		    	}
		    }
		    terminate(curr[num], num);
		}
	}

    pthread_exit(NULL);
}


//=======================================================================================================================================================//
//=========================================================ROUND ROBIN SCHEDULING========================================================================//
//=======================================================================================================================================================//

void context_switch(int num)
{
    pthread_mutex_lock(&preemptMutex2);

    process temp = curr[num];
    curr[num] = dequeue2();
    enqueue2(temp);
    
    a[num] = curr[num].pName;
    contextSwitchCount += 1;

    pthread_mutex_unlock(&preemptMutex2);
}


void* roundRobin(void* args)
{
    int num = *((int*)args);
	while (terminatedCount < procCount)
	{
        sleep(0.1);
		if (!readyQueue.empty())
		{
            contextSwitchCount += 1;
		    curr[num] = dequeue2();
            a[num] = curr[num].pName;
            curr[num].state = 2;
		    double currentTime = timer;
            double temp = timeSlice;
		    while (curr[num].cpuTime > 0) 
		    {
                curr[num].cpuTime -= 0.1;
                print_details(curr[num].state, curr[num].pName);
                sleep(0.1);
                if (readyQueue.size() > 1 && (timer - currentTime) >= timeSlice)
                {
                    context_switch(num);
                    currentTime = timer;
                    timeSlice = temp;
                }
		    	if (curr[num].ioTime>0 && (timer - currentTime)>=curr[num].cpuTime/curr[num].ioTime)
		    	{
                    temp = timeSlice;
                    timeSlice = (timer - currentTime);
                    curr[num].cpuTime -= (timer - currentTime);
		    		yield(curr[num], num);
                    currentTime = timer;
		    	}
		    }
            timeSlice = temp;
		    terminate(curr[num], num);
		}
	}

    pthread_exit(NULL);
}


//=======================================================================================================================================================//
//=========================================================STARTING UP PROCESSING========================================================================//
//=======================================================================================================================================================//


void wakeup(int CPUs, char scheduleMethod, int timeslice = 0)
{
    pthread_mutex_init(&printMutex, NULL);
    cpuThreads = new pthread_t[CPUs];
    pthread_create(&timer_t2, NULL, timerStart, NULL);
    pthread_create(&putinReady, NULL, initReadyQueue, NULL);

    if (scheduleMethod == 'f')
    {
        for (int i = 0; i < CPUs; i++)
        {
            int* num = new int;
            *num = i;
            pthread_create(&cpuThreads[i], NULL, fcfs, num);
        }
    }
    else if (scheduleMethod == 'p')
    {
        curr = new process[CPUs];
        pthread_mutex_init(&preemptMutex, NULL);
        for (int i = 0; i < CPUs; i++)
        {
            int* num = new int;
            *num = i;
            pthread_create(&cpuThreads[i], NULL, priority, num);
        }
    }
    else if (scheduleMethod == 'r')
    {
        curr = new process[CPUs];
        pthread_mutex_init(&preemptMutex2, NULL);
        for (int i = 0; i < CPUs; i++)
        {
            int* num = new int;
            *num = i;
            pthread_create(&cpuThreads[i], NULL, roundRobin, num);
        }
    }
    for (long long i = 0; i < 100000000; i++){ }
    
}


void implement_start(int CPUs, char scheduleMethod, string file, int timeslice = 0)
{
    out.open(file, ios_base::app);

    if (scheduleMethod == 'f')
        out << "===============================================================FCFS=======================================================================\n\n";
    else if (scheduleMethod == 'p')
        out << "===============================================================PRIORITY=======================================================================\n\n";
    else if (scheduleMethod == 'r')
        out << "===============================================================ROUND ROBIN=======================================================================\n\n";

	for (int i = 0; i < processes.size(); i++)
	{
		enqueue1(processes[i]);
	}

	cout << "Time\tRunning\tReady\tWaiting\t";
	out << "Time\tRunning\tReady\tWaiting\t";
	a = new string[CPUs];
	for(int i=0;i<CPUs;i++)
	{
		a[i]="(IDLE)";
	}
	for (int i = 0; i < CPUs; i++)
    {
        cout << "CPU " << i+1 << "\t";
        out << "CPU " << i+1 << "\t";
    }
		
	cout << "< I/O Queue <\n\n";
	cout << "=====\t=======\t=====\t=======\t=====\t=====\t=============\n\n";
	cout<<timer<<"\t"<<"0\t"<<"1\t"<<"0\t";

	out << "< I/O Queue <\n\n";
	out << "=====\t=======\t=====\t=======\t=====\t=====\t=============\n\n";
    out<<timer<<"\t"<<"0\t"<<"1\t"<<"0\t";
	for(int i=0;i<CPUs;i++)
	{
		cout<<a[i]<<"\t";
		out<<a[i]<<"\t";
	}
	cout<<"<<\t<<"<<endl;
	out<<"<<\t<<"<<endl;

	wakeup(CPUs, scheduleMethod, timeslice);

}

//===================================================MAIN PROGRAM=====================================================//

int main(int argc, char *argv[])
{
    string temp;
    ifstream in(argv[1]);
    getline(in, temp);
    cout << " " << endl;
    for (int i = 0; temp[i] != '\n'; i++)
    {
        if (temp[i] == '\t')
            count++;
    }
    count++;
    while (!in.eof())
    {
        struct process currProcess;
        for (int i = 0; i < count; i++)
        {
            if (i == 0)
            {
                in >> currProcess.pName;
            }
            else if (i == 1)
                in >> currProcess.priority;
            else if (i == 2)
                in >> currProcess.arrivalTime;
            else if (i == 3)
                in >> currProcess.procType;
            else if (i == 4)
                in >> currProcess.cpuTime;
            else if (i == 5)
                in >> currProcess.ioTime;
        }
        currProcess.state = 1;      //1 = ready, 2 = running, 3 = waiting, 0 = terminated 
        processes.push_back(currProcess);
        procCount++;
    }
    in.close();

    if (count != 6)
    {
        for (int i = 0; i < procCount; i++)
        {
            processes[i].cpuTime = rand()%10 + 2;
            if (processes[i].procType == 'I')
            {
                processes[i].ioTime = rand()%7 + 2;
            }
            else
                processes[i].ioTime = -1;
        }
    }   


    pthread_mutex_init(&queueMutex1, NULL);
    pthread_mutex_init(&queueMutex2, NULL);
    pthread_mutex_init(&queueMutex3, NULL);

    noOfCpus = argv[2][0] - '0';
    char type = argv[3][0];
    if (type == 'r')
        timeSlice = argv[4][0] - '0';

    string fileStr = argv[argc-1];

    
    implement_start(noOfCpus, type, fileStr, timeSlice);

    cout << "\n\nNumber of Context Switches: " << contextSwitchCount << endl;
    cout << "Total execution Time: " << timer << " s\n";
    cout << "Total time spent in ready state: " << readyTime << " s\n";

    out << "\n\nNumber of Context Switches: " << contextSwitchCount << endl;
    out << "Total execution Time: " << timer << " s\n";
    out << "Total time spent in ready state: " << readyTime << " s\n";

    out << "\n\n";
    out.close();

    return 0;
}