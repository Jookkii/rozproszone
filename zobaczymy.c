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

int rank, n;

pthread_t threadKom;
bool doNothing = true;
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
int req_lamport = -1;

static pthread_mutex_t ack_mutex = PTHREAD_MUTEX_INITIALIZER;
int ack_count = 0;

static pthread_mutex_t gp_mutex = PTHREAD_MUTEX_INITIALIZER;
int global_position = 0;

static pthread_mutex_t cp_mutex = PTHREAD_MUTEX_INITIALIZER;
int pozycja = 0;

static pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;
int pos = -1;

static pthread_mutex_t sendacc_mutex = PTHREAD_MUTEX_INITIALIZER;
int sendaccpair = -1;

void setAccPairFlag(int value){
    pthread_mutex_lock(&sendacc_mutex);
    sendaccpair = value;
    pthread_mutex_lock(&sendacc_mutex);
}

void unsetAccPairFlag(){
    pthread_mutex_lock(&sendacc_mutex);
    sendaccpair = -1;
    pthread_mutex_lock(&sendacc_mutex);
}

int getAccPairFlag(){
    pthread_mutex_lock(&sendacc_mutex);
    int value = sendaccpair;
    pthread_mutex_lock(&sendacc_mutex);
    return value;
}


void setPos(int val){
    pthread_mutex_lock(&cp_mutex);
    pos = val;
    pthread_mutex_unlock(&cp_mutex);
}

int getPos(){
    pthread_mutex_lock(&cp_mutex);
    int val = pos;
    pthread_mutex_unlock(&cp_mutex);
    return val;
}


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

int changeLamport(int val1, int val2){
    pthread_mutex_lock(&req_lamport_mutex);
    if(val1>val2){
        lamport_clock = val1+1;
    }
    else{
        lamport_clock = val2+1;
    };
    pthread_mutex_unlock(&req_lamport_mutex);
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

void zeroACK() {
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
                if(pakiet[1]< getReqLamport()){
                    incrementCurrentPosition(); 
                }
                if(pakiet[1]== getReqLamport() && pakiet[0] < rank){
                    incrementCurrentPosition();
                }
                // printf("%d-Mam pare :%d - na pozycji: %d\n", rank,getPara(),getCurrentPosition());
                
                changeLamport(pakiet[1],getLamportClock());
                msg[0] = rank;
                msg[1] = getLamportClock();
                MPI_Send(msg,3, MPI_INT, status.MPI_SOURCE, ACKPAIR, MPI_COMM_WORLD );
                incrementLamportClock();
                break;
            case ACKRESOURCES: 
                break;
            case ACKPAIR: 
                changeLamport(pakiet[1],getLamportClock());
                incrementACK();
                break;
            case ACCEPTPAIR:
                changeLamport(pakiet[1],getLamportClock());
                // if (getAccPairFlag() == -1){
                //     setPara(pakiet[0]);
                // }
                // unsetAccPairFlag();
                printf("%d: dostałem acceptpari od %d\n", rank, pakiet[0]);
                break;
            case REQRESOURCES: 
                break;
            case RELRESOURCES: 
                break;
            case BROADCASTPOSITION:    
                changeLamport(pakiet[2],getLamportClock());
                if(getACK()==n){
                    if(pakiet[1] == getCurrentPosition()){
                        //printf("nasza pozycja: %d,pozycja w wiadomosci: %d, nasze_id: %d, id_w_wiadomosci: %d \n", getPos(),pakiet[1],rank, pakiet[0]);
                        msg[0] = rank;
                        incrementLamportClock();
                        msg[1] = getLamportClock();
                        printf("%d -wysyłam acceot pair do %d, o czasie:%d \n",rank, pakiet[0],msg[1]);
                        MPI_Send(msg,3,MPI_INT,pakiet[0],ACCEPTPAIR,MPI_COMM_WORLD);
                        //setAccPairFlag(pakiet[0]);
                        setPara(pakiet[0]);
                        zeroACK();
                    }
                }
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
    bool artysta;
    int size;
    MPI_Status status;
    int provided;
    printf("1\n");
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    printf("2\n");
    check_thread_support(provided);
    printf("3\n");
    srand(rank);
    printf("4\n");
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("5\n");
    printf("size:%d\n", size);
    
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
    pthread_create( &threadKom, NULL, startKomWatek , 0);

    while(true){
        if(doNothing){
            unsetPara();
            int n = rand();
            n = n%4;
            sleep(n+4);
            doNothing = false;
            findPosition = true;
        }
        if(findPosition){
            setRequest();
            int msg[3];
            msg[0] = rank;
            incrementLamportClock();
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
            waitForAck = true;
        }
        if(waitForAck){
            //printf("id:%d ack: %d\n", rank, getACK());
            if(getACK() == n){
                waitForAck = false;
                waitForPair = true;
                int msg[3];
                msg[0] = rank;
                msg[1] = getCurrentPosition();
                incrementLamportClock();
                msg[2] = getLamportClock();
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
                char c = 'g';
                if(artysta){
                    c = 'a';
                }
                //printf("%d:%c: koniec wysyłania - pozycja: %d\n", rank,c,getCurrentPosition());
                setPos(msg[1]);     
            }
        }
        if(waitForPair){
            int aktualna_para = getPara();
            if(aktualna_para >= 0){
                printf("%d-Mam pare :%d\n", rank,aktualna_para);
                waitForPair = false;
                doNothing= true;
                unsetPara();
            }
        }
        if(getResources){
            if(artysta){

            }
            else{

            }
        }
        if(inSection){



            
        }
    }

    MPI_Finalize();
}
