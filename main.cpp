//build with at least C++11, uses thread::yield() and atomics instead of semaphores


#include <string>
#include <iostream>
#include <stdlib.h>
#include <unistd.h> //used for sleep(int seconds); //not miliseconds
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "buffer.h" //contains some info for our buffer

using namespace std;

////////////////function declarations////////////////
int insert_item(buffer_item item);  //inserts item into buffer, used by Producers
int remove_item(buffer_item *item); //removes item from buffer, used by Consumers
void *producer();                   //A producer thread will run this function
void *consumer();                   //A consumer thread will run this function
void printBuffer();                 //Prints the contents of the buffer

buffer_item buffer[BUFFER_SIZE];
atomic_int front;
atomic_int back;
atomic_int bufferCount;
mutex *locker;

/*
PURPOSE:This program creates a buffer, creates producer threads, and creates consumer threads.
        Consumer threads take away buffer items from the buffer while producer threads create
        and place buffer items in the buffer.  The program must avoid race conditions by solving
        the critical section problem by applying semaphores.

argv[1] ==> how long main thread sleeps before terminating (seconds)
argv[2] ==> the number of producer threads
argv[3] ==> the number of consumer threads
*/
int main(int argc, char *argv[])
{
    cout << "Michael Lingo, Shane Laskowski" << endl;
    //Get command line arguments argv[1], argv[2], argv[3]
    //Do error checking for bad arguments here
    srand(time(0));
    int sleepyTime = 0;
    int numConsumers = 0;
    int numProducers = 0;
    try
    {
        sleepyTime = stoi(argv[1]);
        numProducers = stoi(argv[2]);
        numConsumers = stoi(argv[3]);
    }
    catch (exception &e)
    {
        cerr << "An invalid argument was entered," << endl
             << "Three arguments are required:" << endl
             << "How long the program is to run in seconds, followed by "
             << "a number of producer threads, followed by "
             << "a number of consumer threads." << endl;
        return -1;
    }
    //Intialize the Buffer with something (random?)
    locker = new mutex();
    front = 0;
    back = 0;
    bufferCount = 0;
    vector<thread *> ProcuerThreads;
    vector<thread *> ConsumerThreads;
    for (int i = 0; i < numProducers; i++)
    {
        //Create Producer threads
        ProcuerThreads.push_back(new thread(producer));
    }

    for (int i = 0; i < numConsumers; i++)
    {
        //Create Consumer threads
        ConsumerThreads.push_back(new thread(consumer));
    }
    //Make main sleep for certain amount of time
    sleep(sleepyTime);
    locker->lock();
    cout << front << back << bufferCount << endl;
    //Exit properly
    return 0;
}

////////////////functions' implementation////////////////

int insert_item(buffer_item itemToAdd)
{
    //insert the item to the buffer, return 0 if successful or -1 for error condition
    while (true)
    {
        if (bufferCount >= BUFFER_SIZE)
            this_thread::yield();
        else
        {
            locker->lock();
            if (bufferCount >= BUFFER_SIZE)
                locker->unlock();
            else
                break;
        }
    }
    buffer[back] = itemToAdd;
    back = (back + 1) % BUFFER_SIZE;
    bufferCount++;
    printBuffer();
    locker->unlock();
    return 0;
}

int remove_item(buffer_item *itemThatWasRemoved)
{
    //removes an item from the buffer, "placed" in *itemThatWasRemoved
    //the item removed is created on the heap ==> once removed we need to keep a track on it
    //use it in main then delete it.
    while (true)
    {
        if (bufferCount <= 0)
            this_thread::yield();
        else
        {
            locker->lock();
            if (bufferCount <= 0)
                locker->unlock();
            else
                break;
        }
    }
    *itemThatWasRemoved = buffer[front];
    front = (front + 1) % BUFFER_SIZE;
    bufferCount--;
    printBuffer();
    locker->unlock();
    //return 0 if successful, else return -1 if error condition
    return 0;
}

void *producer()
{
    buffer_item item;

    while (true)
    {
        //sleep for a random period
        usleep(rand() % 500);

        //generate a random number
        item = rand() % 300;
        if (insert_item(item) == -1) //***remember, 0 results in false, -1 results in true
            cout << "Error: Can't add, the buffer is currently full" << endl;
        //else cout << "Producer created and added " << item << " to the buffer" << endl;
    }
}

void *consumer()
{
    buffer_item item;

    while (true)
    {
        //sleep for random period
        usleep(rand() % 500);

        if (remove_item(&item) == -1) //***remember, 0 results in false, -1 results in true
            cout << "Error: Can't remove, the buffer is currently empty" << endl;
        //else cout << "Consumer consumed and took out " << item << " from the buffer" << endl;
    }
}

void printBuffer()
{

    cout << "[ ";
    for (int i = 0; i < bufferCount; i++)
    {
        cout << buffer[(front + i) % BUFFER_SIZE] << " ";
    }
    cout << "]" << endl;
}