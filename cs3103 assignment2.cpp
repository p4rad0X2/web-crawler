/*
CS3103 OS Assignment 2
Name: Pratul Rajagopalan
SID: 55858290
Commands to run:
g++ 55858290.cpp -lpthread -o main

sample input that I used:
./main 10000 10000

*/
#include <iostream>
#include <pthread.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <semaphore.h>

#include "generator.cpp"

using namespace std;

//string generator function
char *str_generator();

//initialising values for the variables label and article
int label = -1;
int artc = 0;

//initialising values for the variables interval a&b which are read in as inputs from the user
unsigned int intv_a = 0, intv_b = 0;

//assigning value of quit as false
bool QUIT = false;

//initialising array
int arr[13] = {0};

//initialising mutex and semaphore
static pthread_mutex_t mutex, IOmutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t semBuf;

//initialising the file object
static ofstream f1;

//function printstatus that prints the status of each crawler
void printStatus(string str, int id) 
{
    //locking the mutex
    pthread_mutex_lock(&IOmutex);

    //aligning the output depending on the id
    if(id == 0)
    {
        cout <<left<<setfill(' ')<<setw(75)<<' '<<str<<endl;
    }

    else if(id == 1)
    {
        cout <<left<<str<<endl;
    }

    else if(id == 2)
    {
        cout <<left<<setfill(' ')<<setw(25)<<' '<<str<<endl;
    }

    else if(id == 3)
    {
        cout <<left<<setfill(' ')<<setw(50)<<' '<<str<<endl;
    }
    
    //unlocking the mutex once the output has been printed
    pthread_mutex_unlock(&IOmutex);
}

//creating the variables for the buffer
int idfront, totsize;
char **artcl;

//function to initialise the buffer queue
int init1()
{
    int k=1;
    artcl = new char*[12];
    for (int i = 0; i < 12; i++)
    {
        artcl[i] = new char[5];
    }
    idfront = totsize = 0;
    return k;
}
int l = init1();

//function to check if the buffer is empty
bool chkept()
{
    return totsize == 0; 
}

//function to add a character to the article array
void addart(char* ch) 
{ 
    artcl[totsize++] = ch; 
}
//function to return the top most article
char* pop() 
{
    char *art1 = artcl[0];
    for (int i = 0; i < totsize - 1; i++)
        artcl[i] = artcl[i+1];
    artcl[--totsize] = nullptr;
    return art1;
}

//function to check if enough articles have been found
bool termchk() 
{
    int min = arr[0];
    for(int i=1; i<13; i++)
    {
        if(arr[i]<min)
            min = arr[i];
    }
    return min >= 5; 
}

//function to check if the character is a letter
bool letterchk(char ch) 
{ 
    return !isalpha(ch); 
}

//function classifier
void* classifier(void *arg) 
{

    bool checker = false;
    printStatus("start", 0);
    
    //while quit is false and the buffer is not empty
    while (!QUIT || !chkept()) 
    {
        if (chkept() && !QUIT) 
            continue;
        //lock the mutex
        pthread_mutex_lock(&mutex);
        //pop the top most article from the buffer
        char *otxt = pop();
        printStatus("clfy", 0);
        //unlock mutex
        pthread_mutex_unlock(&mutex);

        //init string temp and assign it the value of otext
        string temp = otxt;
        
        //make all the characters of the string lowercase 
        for (int i=0; i<temp.length(); i++)
        {
            temp[i] = tolower(temp[i]);
        }

        //remove characters from the string if they are not letters
        temp.erase(remove_if(temp.begin(), temp.end(), letterchk) , temp.end());

        //output the text to the file
        label = int(temp[0] - 'a') % 13 + 1;
        arr[label - 1]++;
        f1 << ++artc << " " << label << " " << otxt << "\n";

        //check the quit condition
        QUIT = termchk();
        if (QUIT) 
        {
            //if the checker condition has been filled then quit the program
            if (!checker) 
            {
                printStatus(to_string(artc) + "-enough", 0);
                checker = true;
            }
        }

        //sleep for the interval given by the user and call the sem_post function
        usleep(intv_b);
        printStatus("f-clfy", 0);
        sem_post(&semBuf);
    }
    //output the value stored and then quit the program after closing the fliestream
    printStatus(to_string(artc) + "-stored", 0);
    printStatus("quit", 0);
    f1.close();
    return NULL;
}

//function for crawler
void* crawler(void *arg) 
{
    int num = *(int*)arg;
    printStatus("start", num);

    //while the quit condition is not true
    while(!QUIT) 
    {
        //if the buffer is full wait for a slot to open
        if (sem_trywait(&semBuf)) 
        {
            printStatus("wait", num);
            sem_wait(&semBuf);
            printStatus("s-wait", num);

            if (QUIT)
                break;
        }

        //grab the number of the id and then suspend the execution of the calling thread for a value given by the user
        printStatus("grab", num);
        usleep(intv_a);

        //lock the mutex and then add an article from the generator
        pthread_mutex_lock(&mutex);
        addart(str_generator());
        //unlock the mutex
        pthread_mutex_unlock(&mutex);

        //print that the article has been grabbed
        printStatus("f-grab", num);
    }

    //when quit is true, print quit and id and then exit the program
    printStatus("quit", num);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
{
    //initialise the intervals a and b
    intv_a = strtoul(argv[1], nullptr, 10);
    intv_b = strtoul(argv[2], nullptr, 10);

    //initialise the 3 crawler threads and the classifier thread
    pthread_t crawlTh[3], classiTh;
    int flag = 0, ct = 0;
    
    //open the file textfilecorpus
    f1.open("textfilecorpus.txt");

    //print the output headings of crawlers 1,2,3 and classifier in the form of a table
    cout <<setw(25)<<setfill(' ')<<left<<"Crawler-1";
    cout <<setw(25)<<setfill(' ')<<left<<"Crawler-2";
    cout <<setw(25)<<setfill(' ')<<left<<"Crawler-3";
    cout <<setw(25)<<setfill(' ')<<left<<"Classifier\n";

    //initialise the semaphore
    sem_init(&semBuf, 0, 12);

    //error handling to handle an error while trying to create the thread
    flag = pthread_create(&classiTh, NULL, classifier, NULL);
    if (flag) {
        cout << "An error occured while trying to create the thread, the program will exit\n";
        exit(EXIT_FAILURE);
    }

    //error handling to handle an error while creating the thread
    for (int i = 0; i < 3; i++) {
        ct = i + 1;
        flag = pthread_create(&crawlTh[i], NULL, crawler, (void *)&ct);
        if (flag) {
            cout << "An error occured while trying to create the thread, the program will exit\n";
            exit(EXIT_FAILURE);
        }
    }

    //error handling to handle an error that occurs while trying to join a thread
    for (int i = 0; i < 3; i++) {
        ct = i + 1;
        flag = pthread_join(crawlTh[i], NULL);
        if (flag) {
            cout << "An error occured while trying to join the thread, the program will exit\n";
            exit(EXIT_FAILURE);
        }
    }

    //error handling to handle an error that occurs while trying to join a thread
    flag = pthread_join(classiTh, NULL);
    if (flag) {
        cout << "An error occured while trying to join the thread, the program will exit\n";
        exit(EXIT_FAILURE);
    }

    return 0;
    
}