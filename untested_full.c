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
int resources = 20;
int rank, n;

pthread_t threadKom;
bool doNothing = true;
bool findPosition = false;
bool waitForPair = false;
bool waitForAck = false;
bool getResources = false;
bool inSection = false;

ReqQue ReqQ;

static pthread_mutex_t para_mutex = PTHREAD_MUTEX_INITIALIZER;
int para = -1;

typedef struct Data {
    int lamport;
    int id;
    int res_num;
    bool ack;
} Data;

typedef struct Node {
    Data data;
    struct Node *previous;         
    struct Node *next;
} Node;

typedef struct ReqQue {
    Node *head;
} ReqQue;

void add_node(int lamp, int id, int res_num)
{
    Node* node = malloc(sizeof(Node));
    node->data.lamport = lamp;
    node->data.id = id;
    node->data.res_num = res_num;
    node->data.ack = false;
    node->previous = NULL;
    node->next = NULL;
    if(ReqQ.head == NULL)
    {
        ReqQ.head = node;
        return;
    }
    //printf("Something's fucky\n");
    Node* current = ReqQ.head;
    if(node->data.lamport<ReqQ.head->data.lamport||(node->data.lamport == ReqQ.head->data.lamport && node->data.id<ReqQ.head->data.id))
    {
        ReqQ.head = node;
        node->next = current;
        current->previous = node;
        return;  
    }
    while(current->next!=NULL)
    {
        current=current->next;
        if(node->data.lamport<current->data.lamport || (node->data.lamport==current->data.lamport&&node->data.id==current->data.id))
        {
            node->previous = current->previous;
            current->previous->next = node;
            current->previous = node;
            node->next = current;
            return;
        }
    }
    current->next = node;
    node->previous = current;
}
void remove_request(int id)
{
    if(ReqQ.head == NULL)
    {
        exit(-1);
    }
    Node* current=ReqQ.head;
    if(current->data.id==id)
    {
        ReqQ.head = current->next;
        free(current);
        return;
    }
    printf("A\n");
    while(current->next!=NULL)
    {
        current = current->next;
        if(current->data.id==id)
        {
            if(current->next!=NULL)
            {
                current->next->previous = current->previous;
            }
            if(current->previous!=NULL)
            {
                current->previous->next = current->next;
            }
            free(current);
            return;
        }
    }
}
// int parseQ(int max_res)
// {
//     if(ReqQ.head==NULL)
//     {
//         printf("Are u stoopid?\n");
//         exit(-1);
//     }
//     Node* current = ReqQ.head;
//     printf("----------------\n");
//     int res=0;
//     while(res+current->data.res_num<max_res)
//     {
//         res+=current->data.res_num;
//         //printf("Node lamp: %d, Node id: %d, Node resources: %d\n",current->data.lamport, current->data.id,current->data.res_num);
//         if(current->next==NULL)
//         {
//             return res;
//         }
//         current = current->next;
//     }
//     return res;
// }
void send_acks(int res)
{
    int msg[3];
    Node* current = ReqQ.head;
    int curr_res=0;
    while(current!=NULL&&current->data.res_num+curr_res<=res)
    {
            msg[0]=rank;
            msg[1]=lamport_clock;
            msg[2]=1;
            curr_res+=current->data.res_num;
            if(current->data.ack==false)
            {
                current->data.ack=true;
                MPI_Send(msg,3, MPI_INT, current->data.id, ACKRESOURCES, MPI_COMM_WORLD );
            }  
    }
}
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

static pthread_mutex_t ack_res_mutex = PTHREAD_MUTEX_INITIALIZER;
int ack_res_count = 0;

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
//ACK resources counter
int getACKres() {
    pthread_mutex_lock(&ack_res_mutex);
    int clock_value = ack_res_count;
    pthread_mutex_unlock(&ack_res_mutex);
    return clock_value;
}

void zeroACKres() {
    pthread_mutex_lock(&ack_res_mutex);
    ack_res_count = 0;
    pthread_mutex_unlock(&ack_res_mutex);
}

void incrementACKres() {
    pthread_mutex_lock(&ack_res_mutex);
    ack_res_count += 1;
    pthread_mutex_unlock(&ack_res_mutex);
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

void critical_section()
{
    sleep(rand()%5+2);
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
        changeLamport(pakiet[1],getLamportClock());
        incrementLamportClock();
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
                
                
                msg[0] = rank;
                incrementLamportClock();
                msg[1] = getLamportClock();
                MPI_Send(msg,3, MPI_INT, status.MPI_SOURCE, ACKPAIR, MPI_COMM_WORLD );
                break;
            case ACKRESOURCES:
                incrementACKres();
                break;
            case ACKPAIR: 
                incrementACK();
                break;
            case ACCEPTPAIR:
                // if (getAccPairFlag() == -1){
                //     setPara(pakiet[0]);
                // }
                // unsetAccPairFlag();
                printf("%d: dostałem acceptpari od %d\n", rank, pakiet[0]);
                break;
            case REQRESOURCES:
                add_node(pakiet[1],pakiet[0],pakiet[2]);
                send_acks(resources);
                break;
            case RELRESOURCES:
                remove_request(pakiet[0]);
                send_acks(resources);
                break;
            case BROADCASTPOSITION:    
                if(getACK()==n){
                    if(pakiet[2] == getCurrentPosition()){
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
                getResources=false;
                inSection=true;
                break;
            case LEAVESECTION:
                incrementLamportClock();
                for(int i=0;i<size;i++)
                {
                    MPI_Send(msg,3,MPI_INT,i,RELRESOURCES,MPI_COMM_WORLD);
                }
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
                incrementLamportClock();
                msg[1] = getLamportClock();
                msg[2] = getCurrentPosition();
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
                setPos(msg[2]);     
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
                zeroACKres()
                msg[0]=rank;
                incrementLamportClock();
                msg[1]=getLamportClock();
                msg[2]=rand()%4+1;
                for(int i=0;i<size;i++)
                {
                    MPI_Send( msg,3, MPI_INT, i, REQRESOURCES, MPI_COMM_WORLD );
                }
                while(getACKres()!=size)
                {
                    sleep(1);
                }
                getResources=false;
                inSection=true;
            }
            
        }
        if(inSection){
            if(artysta)
            {
                msg[0]=rank;
                incrementLamportClock();
                msg[1]=getLamportClock();
                msg[2]=0;
                MPI_Send(msg,3,MPI_INT,getPara(),INSECTION,MPI_COMM_WORLD);
                critical_section();
            }
            else
            {
                critical_section();
                msg[0]=rank;
                incrementLamportClock();
                msg[1]=getLamportClock();
                msg[2]=0;
                MPI_Send(msg,3,MPI_INT,getPara(),LEAVESECTION,MPI_COMM_WORLD);
            }
            unsetPara();
            inSection=false;
            doNothing=true;
        }
    }

    MPI_Finalize();
}
