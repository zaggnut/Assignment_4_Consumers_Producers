//build with at least C++11, uses thread::yield() and atomics instead of semaphores

#include <string>
#include <iostream>
#include <stdlib.h>
#include <unistd.h> //used for sleep(int seconds); //not miliseconds
#include <thread>
#include <vector>
#include <mutex>
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
atomic_int front;       //used to indicate the index of 1st buffer slot that isn't empty
atomic_int back;        //used to inidcate the index of the last buffer slot that isn't empty
atomic_int bufferCount; //counts the number of items within the buffer, used to check if buffer is full or empty
mutex *locker;          //mutex lock to help avoid race conditions

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

    //initialization of variables (the buffer is empty at this point)
    locker = new mutex();
    front = 0;
    back = 0;
    bufferCount = 0;

    //creation of threads
    vector<thread *> ProducerThreads;
    vector<thread *> ConsumerThreads;
    cout << "Spawning threads" << endl;
    sleep(2);
    for (int i = 0; i < numProducers; i++)
    {
        //Create Producer threads
        ProducerThreads.push_back(new thread(producer));
    }

    for (int i = 0; i < numConsumers; i++)
    {
        //Create Consumer threads
        ConsumerThreads.push_back(new thread(consumer));
    }

    //Main will sleep, not wait, for certain amount of time
    sleep(sleepyTime);
    locker->lock(); //main will hold the lock now, preventing anymore threads from working in their critical sections
    cout << "The buffer ended at size: " << bufferCount << endl;

    return 0; //now this process ends and kills all threads along with it
}

////////////////functions' implementation////////////////

//inserts an item into the buffer for any consumer thread that calls this function
int insert_item(buffer_item itemToAdd)
{
    //insert the item to the buffer, return 0 if successful or -1 for error condition
    while (true)
    {
        if (bufferCount >= BUFFER_SIZE) //if buffer is full, do busy waiting
        {
#if defined __i386__ || __amd64__
            asm volatile("pause"); //preprocessor directive, this improves performance on spin wait loops
#endif
            this_thread::yield();
        }
        else
        {
            locker->lock(); //mutex lock for critical section
            if (bufferCount >= BUFFER_SIZE)
                locker->unlock(); //unlock the mutex lock, because there is a chance that another thread had messed with buffer after that if() statement above
            else
                break;
        }
    }
    //critical section
    buffer[back] = itemToAdd;
    back = (back + 1) % BUFFER_SIZE;                //makes the back variable "point" to the next index of the buffer that is to get the next inserted item
    bufferCount.fetch_add(1, memory_order_relaxed); //bufferCount is an atomic variable ==> need to use .fetch_add to add 1 to its value
    cout << "The item " << itemToAdd << " was inserted" << endl;
    printBuffer(); //prints the buffer to see the changes made
    //end of critical section
    locker->unlock();
    return 0;
}

//removes an item for the consumer thread that calls this function
int remove_item(buffer_item *itemThatWasRemoved)
{
    //removes an item from the buffer, "placed" in *itemThatWasRemoved
    //the item removed is created on the heap ==> once removed we need to keep a track on it
    //use it in main then delete it.
    while (true)
    {
        if (bufferCount <= 0)
        {
#if defined __i386__ || __amd64__
            asm volatile("pause"); //this improves performance on spin wait loops
#endif
            this_thread::yield();
        }
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
    bufferCount.fetch_add(-1, memory_order_relaxed);
    cout << "The item " << *itemThatWasRemoved << " was removed" << endl;
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
        if (insert_item(item) == -1) //insert_item returns 0 if successful, -1 for failure
            cout << "Error: Can't add, the buffer is currently full" << endl;
        //else cout << "Producer created and added " << item << " to the buffer" << endl; //used for testing
    }
}

void *consumer()
{
    buffer_item item;

    while (true)
    {
        //sleep for random period
        usleep(rand() % 500);

        if (remove_item(&item) == -1)
            cout << "Error: Can't remove, the buffer is currently empty" << endl;
        //else cout << "Consumer consumed and took out " << item << " from the buffer" << endl; //used for testing
    }
}

//prints the buffer out, but only prints the indexes with items in them (items that are at and between Front and Back, the rest are just junk)
void printBuffer()
{

    cout << "[ ";
    for (int i = 0; i < bufferCount; i++)
    {
        cout << buffer[(front + i) % BUFFER_SIZE] << " ";
    }
    cout << "]" << endl;
}