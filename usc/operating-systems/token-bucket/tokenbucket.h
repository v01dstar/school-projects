#include <sys/time.h>
#include "cs402.h"
#include "my402list.h"


#ifndef DETERMINISTIC
#define DETERMINISTIC 0
#define TRACEDRIVEN 1
#endif /* RUNNING MODE */


#ifndef MAXLINESIZE
#define MAXLINESIZE 1024
#endif


typedef struct tagPacket {
    int packet_no;
    int token_num;
    long inter_arrival;
    long serve_time;
    long time_in_system;
    long time_in_Q1;
    long time_in_Q2;
    struct timeval timeline[7];
} Packet;


typedef struct tagConfig {
    int mode;
    double lambda;
    double mu;
    double r;
    int B;
    int P;
    int n;
    char file[MAXPATHLENGTH];
    FILE *tsfile;
} Config;


typedef struct tagStatistics {
    int completed_packets;
    int dropped_packets;
    int removed_packets;
    int token;
    int dropped_token;
    double avg_inter_arrival;
    double avg_serve_time;
    double avg_time_in_system;
    double avg_time_square;
    double time_in_Q1;
    double time_in_Q2;
    double time_in_S1;
    double time_in_S2;
} Statistics;


/* Threads declaration*/
extern void *PacketArrival(void*);
extern void *TokenDepositing(void*);
extern void *PacketServing(void*);
extern void *monitor(void *arg);


/*Functions in threads*/
extern Packet *CreatePacket(int, struct timeval*, int, long, int);
extern long TimeGap(struct timeval, struct timeval);
extern void Enqueue(My402List*, Packet*);
extern Packet *Dequeue(My402List*);
extern int IsEmpty(My402List*);
extern int IsEnoughToken();
extern struct timeval IncreaseToken(int, int);
extern void ParseLine(char*, long*, int*, long*);
extern char *SeparateLine(char*);
extern void PrintTime(struct timeval);
extern void Sleep(struct timeval, long);
extern void Serve(Packet*, int);
extern void GetLine(FILE*, char[]);
extern void DisplayParameters(Config*);
extern void DisplayStat(Statistics);
extern void Cleanup(My402List *);
extern void UnlockMutex(void*);
