/* w main.h także makra println oraz debug -  z kolorkami! */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define REQPOSITION 100 //--
#define ACKPAIR 102 //--
#define BROADCASTPOSITION 106 //--
#define ACCEPTPAIR 103

#define ACKRESOURCES 101 
#define REQRESOURCES 104
#define RELRESOURCES 105

#define INSECTION 107
#define LEAVESECTION 108

int artysci = 3;

pthread_t threadKom;
bool doNothing = false;
bool findPosition = false;
bool waitForPair = false;
bool waitForAck = false;
bool getResources = false;
bool inSection = false;



static pthread_mutex_t para_mutex = PTHREAD_MUTEX_INITIALIZER;
int para = -1;

void setPara(int value){
    pthread_mutex_lock(&para_mutex);
    para = value;
    pthread_mutex_unlock(&para_mutex);
}

int getPara(){
    pthread_mutex_lock(&para_mutex);
    int value = para;
    pthread_mutex_unlock(&para_mutex);
    return value;
}

void unsetPara(){
    pthread_mutex_lock(&para_mutex);
    para = -1;
    pthread_mutex_unlock(&para_mutex);
}


static pthread_mutex_t lamport_clock_mutex = PTHREAD_MUTEX_INITIALIZER;
static int lamport_clock = 0; 

static pthread_mutex_t req_lamport_mutex = PTHREAD_MUTEX_INITIALIZER;
int req_lamport = 0;

static pthread_mutex_t ack_mutex = PTHREAD_MUTEX_INITIALIZER;
int ack_count = 0;


static pthread_mutex_t gp_mutex = PTHREAD_MUTEX_INITIALIZER;
int global_position = 0;

static pthread_mutex_t cp_mutex = PTHREAD_MUTEX_INITIALIZER;
int pozycja = 0;


void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}



//CurrentPosition
int getCurrentPosition(){
    pthread_mutex_lock(&cp_mutex);
    int current = pozycja;
    pthread_mutex_unlock(&cp_mutex);
    return current;
}

void setCurrentPosition(int value){
    pthread_mutex_lock(&cp_mutex);
    pozycja = value;
    pthread_mutex_unlock(&cp_mutex);
}

void incrementCurrentPosition(){
    pthread_mutex_lock(&cp_mutex);
    pozycja += 1;
    pthread_mutex_unlock(&cp_mutex);
}


void setRequest(){
    pthread_mutex_lock(&gp_mutex);
    pthread_mutex_lock(&cp_mutex);
    pthread_mutex_lock(&ack_mutex);
    pthread_mutex_lock(&lamport_clock_mutex);
    pthread_mutex_lock(&req_lamport_mutex);
    pozycja = global_position;
    req_lamport = lamport_clock;
    ack_count = 0;
    pthread_mutex_unlock(&req_lamport_mutex);
    pthread_mutex_unlock(&lamport_clock_mutex);
    pthread_mutex_unlock(&ack_mutex);
    pthread_mutex_unlock(&cp_mutex);
    pthread_mutex_unlock(&gp_mutex);
}



//GlobalPosition
int GetGlobalPosition(){
    pthread_mutex_lock(&gp_mutex);
    int current = global_position;
    pthread_mutex_unlock(&gp_mutex);
    return current;
}

void IncrementGlobalPosition(){
    pthread_mutex_lock(&gp_mutex);
    global_position+=1;
    pthread_mutex_unlock(&gp_mutex);
}

//ACKCounter
int getACK() {
    pthread_mutex_lock(&ack_mutex);
    int clock_value = ack_count;
    pthread_mutex_unlock(&ack_mutex);
    return clock_value;
}

void zeroACK(int value) {
    pthread_mutex_lock(&ack_mutex);
    ack_count = 0;
    pthread_mutex_unlock(&ack_mutex);
}

void incrementACK() {
    pthread_mutex_lock(&ack_mutex);
    ack_count += 1;
    pthread_mutex_unlock(&ack_mutex);
}

//RequestLamportClock
int getReqLamport() {
    pthread_mutex_lock(&req_lamport_mutex);
    int clock_value = req_lamport;
    pthread_mutex_unlock(&req_lamport_mutex);
    return clock_value;
}

void setReqLamport(int value) {
    pthread_mutex_lock(&req_lamport_mutex);
    req_lamport = value;
    pthread_mutex_unlock(&req_lamport_mutex);
}

//LamportClock
int getLamportClock() {
    pthread_mutex_lock(&lamport_clock_mutex);
    int clock_value = lamport_clock;
    pthread_mutex_unlock(&lamport_clock_mutex);
    return clock_value;
}

void incrementLamportClock() {
    pthread_mutex_lock(&lamport_clock_mutex);
    lamport_clock++;
    pthread_mutex_unlock(&lamport_clock_mutex);
}

void setLamportClock(int value) {
    pthread_mutex_lock(&lamport_clock_mutex);
    lamport_clock = value;
    pthread_mutex_unlock(&lamport_clock_mutex);
}

//Wątek komunikacyjny
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int pakiet[3];
    int msg[3];
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while (true) {
        MPI_Recv( &pakiet, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch ( status.MPI_TAG ) {
            case REQPOSITION:             
                IncrementGlobalPosition();
                if(pakiet[1]< getLamportClock()){
                    incrementCurrentPosition();
                }
                if(pakiet[1]>getLamportClock()){
                    setLamportClock(pakiet[1]);
                }
                incrementLamportClock();
                //podnieś globalną pozycje
                //podnieś wlasną pozycje
                //wyślij

                MPI_Send(msg,3, MPI_INT, status.MPI_SOURCE, ACKPAIR, MPI_COMM_WORLD );
                break;
            case ACKRESOURCES: 
                break;
            case ACKPAIR: 
                incrementACK();
                break;
            case ACCEPTPAIR: 
                break;
            case REQRESOURCES: 
                break;
            case RELRESOURCES: 
                break;
            case BROADCASTPOSITION: 
                break;
            case INSECTION: 
                break;
            case LEAVESECTION: 
                break;
            default:
            break;
        }
    }
}

int main(int argc, char **argv)
{   
    int n;
    bool artysta;
    int rank,size;
    MPI_Status status;
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    
    check_thread_support(provided);
    srand(rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("size:%d", size);
    if(size<artysci){
        exit(-1);
    }
    if(rank<artysci){
        artysta = true;
        n = artysci;
    }
    else{
        n = size-artysci;
    }
    //pthread_create( &threadKom, NULL, startKomWatek , 0);

    while(true){
        printf("something");
        if(doNothing){
            int n = rand();
            n = n%10;
            sleep(n);
            doNothing = false;
            findPosition = true;
        }
        if(findPosition){
            setRequest();
            int msg[3];
            msg[0] = rank;
            msg[1] = getReqLamport();
            msg[2] = 0;
            if(artysta){
                for(int i = 0; i < artysci; i++){
                    MPI_Send( msg,3, MPI_INT, i, REQPOSITION, MPI_COMM_WORLD );
                }
            }
            else{
                for(int i = artysci; i < size; i++){
                    MPI_Send( msg,3, MPI_INT, i, REQPOSITION, MPI_COMM_WORLD );
                }
            }    
            findPosition = false;
            waitForPair = true;
        }
        if(waitForAck){
            if(getACK() == n){
                waitForAck = false;
                waitForPair = true;
                int msg[3];
                msg[0] = rank;
                msg[1] = getCurrentPosition();
                msg[2] = 0;
                if(artysta){
                    for(int i = artysci; i < size; i++){
                        MPI_Send( msg,3, MPI_INT, i, BROADCASTPOSITION, MPI_COMM_WORLD );
                    }
                }
                else{
                    for(int i = 0; i < artysci; i++){
                        MPI_Send( msg,3, MPI_INT, i, BROADCASTPOSITION, MPI_COMM_WORLD );
                    }
                }     
            }
        }
        if(waitForPair){
            if(getPara() >= 0){
                getResources= true;
                waitForPair = false;
            }
        }
        if(getResources){
            if(artysta){

            }
            else{

            }
        }
        if(inSection){



            unsetPara();
        }
    }

    MPI_Finalize();
}