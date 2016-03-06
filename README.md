# lock-based-and-wait-free-queue
This is an implementation of lock based and wait free queue in C++
**Lock Based and Wait Free Queue implementations**
Input file must be named "inp-params.txt" with parameters N, EQCount, DQCount, Lambda1, Lambda2
To execute the Lock Based queue and test its performance type the following on the terminal
$ g++ lockbasedqueue.cpp -lpthread -std=c++11
$ ./a.out

To exeute the Wait Free Queue and test its performance type the following in the terminal
$ g++ queuewaitfree.cpp -lpthread -std=c++11
$ ./a.out

Output:

The output files are "output.txt" which gives the real time execution order and "serial.txt" which
gives the equivalent sequential history
