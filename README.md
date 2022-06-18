# OS-Kernel-Simulator-CPP
Kernel simulator in Unix system using C++
Operating system kernel simulation is created in unix system using system calls to acces pthreads and kernel operations.
Data is entered in command line as: "[Binary output File name] [Input File] [No. of CPUS] [Type of Scheduling] [Timeslice in case of round robin] [Output File]".
The program reads data from the input files and initialises PCB for all the processes mentioned.
These processes are then sent into a new queue which is mutex protected.
User can use three types of scheduling: f for first come first serve, r for round robin and p for priority.
After getting the type of scheduling, threads are created according to the number of cpus specified in command line.
Another thread acts as a timer for our work and one other thread is responsible for putting processes into read queue when their arrival time is here.
Then in our main functions (for all three types) it runs until all processes have been terminated.
It checks on each cycle whenever a process is available in ready queue, it picks it up and runs it for it's cpu burst time. When it is completed, process is terminated and next process begins. This goes for each cpu (acted by thread).
During running, a process also goes for I/O if it is an I/O bound process. In this it is placed in waiting queue and during it's I/O time, that CPU remains IDLE. When it's done, it comes back to cpu where it's processing continues.
In First come First serve, as processes keep coming they are processed as the name suggests.
In priority, on each cycle, the whole ready queue is checked and if there is a process with higher priority than current running process, it is pre-empted and higher priority processes run first and lower ones at the end.
In round robin, each process has a time slice for example if it is 2s, every process will run 2 seconds and then get replaced. Like this all processes will run until they're all terminated.
At the end, in all cases, a gantt chart is printed on console which prints timer and processes along with their states and which cpu runs which process at a given time.
This gantt chart is also written to txt file specified in input as Output file.
All queues were made thread safe using mutexes to avoid race condition and synchronisation was maintained between all the threads.
Here the simulation ends.
