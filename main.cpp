#include <string>
#include <iostream>
#include <stdlib.h>
#include <unistd.h> //used for sleep(int seconds); //not miliseconds
#include <pthread.h>
#include <mutex>
#include <semaphore.h>
#include "buffer.h" //contains some info for our buffer
//include the pthread libary

using namespace std;

////////////////function declarations////////////////
int insert_item(buffer_item item);  //inserts item into buffer, used by Producers
int remove_item(buffer_item *item); //removes item from buffer, used by Consumers
void *producer(void *param);         //A producer thread will run this function
void *consumer(void *param);         //A consumer thread will run this function

buffer_item buffer[BUFFER_SIZE];
sem_t Empty;
sem_t Full;
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
    //Get command line arguments argv[1], argv[2], argv[3]
    //Do error checking for bad arguments here
    srand(time(0));
    int sleepyTime = stoi(argv[1]);
    int numProducers = stoi(argv[2]);
    int numConsumers = stoi(argv[3]);

    //Intialize the Buffer with something (random?)
    locker = new mutex();
    sem_init(&Empty, 0, BUFFER_SIZE);
    sem_init(&Full, 0, 0);
    
    for (int i = 0; i < numProducers; i++)
    {
        //Create Producer threads
        pthread_t thread;
        pthread_create(&thread, NULL, producer, NULL);
    }
    
    for (int i = 0; i < numConsumers; i++)
    {
        //Create Consumer threads
        pthread_t thread;
        pthread_create(&thread, NULL, consumer, NULL);
    }
    //Make main sleep for certain amount of time
    sleep(sleepyTime);
    locker->lock();
    int fullcount, emptycount;
    sem_getvalue(&Full, &fullcount);
    sem_getvalue(&Empty, &emptycount);
    cout << fullcount << emptycount << endl;
    //Exit properly
    return 0;
}

////////////////functions' implementation////////////////

int insert_item(buffer_item itemToAdd)
{
    //insert the item to the buffer, return 0 if successful or -1 for error condition
    if (sem_wait(&Empty) != 0)
        return -1;
    locker->lock();
    int index;
    sem_getvalue(&Full, &index);
    buffer[index] = itemToAdd;
    locker->unlock();
    sem_post(&Full);
    return 0;
}

int remove_item(buffer_item *itemThatWasRemoved)
{
    //removes an item from the buffer, "placed" in *itemThatWasRemoved
    //the item removed is created on the heap ==> once removed we need to keep a track on it
    //use it in main then delete it.
    if (sem_wait(&Full) != 0)
        return -1;
    locker->lock();
    int index;
    sem_getvalue(&Full, &index);
    *itemThatWasRemoved = buffer[index];
    locker->unlock();
    sem_post(&Empty);
    //return 0 if successful, else return -1 if error condition
    return 0;
}

void *producer(void *param)
{
    buffer_item item;

    while (true)
    {
        //sleep for a random period
        usleep(rand() % 500);

        //generate a random number
        item = rand();
        if (insert_item(item)) //***remember, 0 results in false, -1 results in true
            cout << "Error: Can't add, the buffer is currently full" << endl;
        else
            cout << "Producer created and added " << item << " to the buffer" << endl;
    }
}

void *consumer(void *param)
{
    buffer_item item;

    while (true)
    {
        //sleep for random period
        usleep(rand() % 500);

        if (remove_item(&item)) //***remember, 0 results in false, -1 results in true
            cout << "Error: Can't remove, the buffer is currently empty" << endl;
        else
            cout << "Consumer consumed and took out " << item << " from the buffer" << endl;
    }
}