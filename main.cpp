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
/*
AUTHORS: Michael Lingo + Shane Laskowski
PURPOSE:This program creates a buffer, creates producer threads, and creates consumer threads.
        Consumer threads take away buffer items from the buffer while producer threads create
        and place buffer items in the buffer.  The program must avoid race conditions by solving
        the critical section problem by applying semaphores.
KEYNOTES: global variables are shared by all threads, semahpores and their values can be used
          to manage a shared buffer between threads.
*/

////////////////function declarations////////////////
int insert_item(buffer_item item);  //inserts item into buffer, used by Producers
int remove_item(buffer_item *item); //removes item from buffer, used by Consumers
void *producer(void *param);        //A producer thread will run this function
void *consumer(void *param);        //A consumer thread will run this function

////////////////global variables////////////////to be shared by all functions/threads
buffer_item buffer[BUFFER_SIZE];    //this array is a shared buffer shared by all threads
sem_t Empty;                        //semaphore variable that regulates producer threads
sem_t Full;                         //semaphore variable that limits that regulates consumer threads
mutex *locker;                      //used to ensure critical section is ran atomically, same thing as a binary semaphore

/*
argv[1] ==> how long main thread sleeps before terminating (seconds)
argv[2] ==> the number of producer threads
argv[3] ==> the number of consumer threads
*/
int main(int argc, char *argv[])
{
    //Get command line arguments argv[1], argv[2], argv[3]
    srand(time(0));

    //converts the program's arguments (strings) into integers and put inside variables for later use
    int sleepyTime = stoi(argv[1]);
    int numProducers = stoi(argv[2]);
    int numConsumers = stoi(argv[3]);

    //Intialize the Buffer with something (random?)
    locker = new mutex();
    sem_init(&Empty, 0, BUFFER_SIZE); //initializes the semahpore, cannot be shared between processes, value is BUFFER_SIZE
    sem_init(&Full, 0, 0);            //initializes the semaphore, cannot be shared between processes, initialized to 0 but will increase whenever a producer thread adds to the shared buffer
    
    //creates numProducers amount of threads, these threads will run the producer function
    for (int i = 0; i < numProducers; i++)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, producer, NULL);
    }
    
    //creates numConsumers amount of threads, these threads will run the consumer function
    for (int i = 0; i < numConsumers; i++)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, consumer, NULL);
    }

    //Make main sleep for certain amount of time, then after main wakes up,
    //it will display the values of the two semaphores Full and Empty and display them
    //main then exits (killing all the threads with it)
    sleep(sleepyTime);
    locker->lock();
    int fullcount, emptycount;
    sem_getvalue(&Full, &fullcount);
    sem_getvalue(&Empty, &emptycount);
    cout << fullcount << emptycount << endl;
    return 0;
}

////////////////functions' implementation////////////////

//inserts an item to the buffer, return 0 if successful or -1 for error condition
int insert_item(buffer_item itemToAdd)
{
    if (sem_wait(&Empty) != 0) //if failure sem_wait returns -1, otherwise returns 0 if successful lock
        return -1;             //failure to add, buffer is full
    
    locker->lock();
    //critical section below this line

    int index;
    sem_getvalue(&Full, &index); //will attempt to change the value at &index to the value of the Full semaphore
    buffer[index] = itemToAdd;   //******what if semaphore value is 5 and buffer[5] (out of bounds) is attempted to be accessed?
    //this line is end of critical section

    locker->unlock();
    sem_post(&Full);
    return 0; //success
}

//removes an item from the buffer, "placed" in *itemThatWasRemoved
//the item removed is created on the heap ==> once removed we need to keep a track on it
int remove_item(buffer_item *itemThatWasRemoved)
{

    if (sem_wait(&Full) != 0)
        return -1; //error, buffer is empty
    
    locker->lock();
    //below is critical section

    int index;
    sem_getvalue(&Full, &index); //attempts to change the value at &index to the value of the semaphore Full
    *itemThatWasRemoved = buffer[index]; //retrieves the item to be removed
    //this line is end of critical section
    
    locker->unlock();
    sem_post(&Empty);
    
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

        //******shouldn't ! (NOT) be placed in front of insert_item(item)
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

        //*****shouldn't ! (NOT) be placed in front of remove_item(&item)
        if (remove_item(&item)) //***remember, 0 results in false, -1 results in true
            cout << "Error: Can't remove, the buffer is currently empty" << endl;
        else
            cout << "Consumer consumed and took out " << item << " from the buffer" << endl;
    }
}