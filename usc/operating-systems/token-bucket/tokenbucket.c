#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include "my402list.h"
#include "tokenbucket.h"


static char gszProgName[MAXPATHLENGTH];
static pthread_t thread_p, thread_b, thread_s1, thread_s2;
static pthread_mutex_t gsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gsCV = PTHREAD_COND_INITIALIZER;
static int gsTokenBucket = 0;    // Global variable of token bucket
static My402List Q1, Q2;
static struct timeval gsBeginTime, gsEndTime;
static Statistics stat = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static sigset_t set;
static int gsFinish = FALSE;

/* ----------------------- Utility Functions ----------------------- */
static void Usage()
{
    fprintf(stderr, "Malformed command\n");
    fprintf(stderr, "usage: %s [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n", gszProgName);
    exit(-1);
}


static Config ProcessOptions(int argc, char *argv[])
{
    Config config = {DETERMINISTIC, 1, 0.35, 1.5, 10, 3, 20};
    for (argc--, argv++; argc>0; argc--, argv++) {
        if (*argv[0] == '-') {
            if (strcmp(*argv, "-lambda") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.lambda = atof(*argv);
                if (config.lambda == 0)
                    Usage();
            } else if (strcmp(*argv, "-mu") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.mu = atof(*argv);
                if (config.mu == 0)
                    Usage();
            } else if (strcmp(*argv, "-r") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.r = atof(*argv);
                if (config.r == 0)
                    Usage();
            } else if (strcmp(*argv, "-B") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.B = atoi(*argv);
                if (config.B == 0)
                    Usage();
            } else if (strcmp(*argv, "-n") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.n = atoi(*argv);
                if (config.n == 0)
                    Usage();
            } else if (strcmp(*argv, "-P") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.P = atoi(*argv);
                if (config.P == 0)
                    Usage();
            } else if (strcmp(*argv, "-t") == 0) {
                argc--;
                if (argc == 0)
                    Usage();
                argv++;
                config.mode = TRACEDRIVEN;
                strncpy(config.file, *argv, strlen(*argv));
            } else {
                Usage();
            }
        } else {
            Usage();
        }
    }
    return config;
}


static void SetProgramName(char *s)
{
    char *c_ptr=strrchr(s, DIR_SEP);

    if (c_ptr == NULL) {
        strcpy(gszProgName, s);
    } else {
        strcpy(gszProgName, ++c_ptr);
    }
}


/* ------------------- Process -------------------------  */
void Process(Config *config)
{
    pthread_t thr;
    char line[MAXLINESIZE];
    if (config->mode == TRACEDRIVEN) {
        if ((config->tsfile = fopen(config->file, "r")) == NULL) {
            fprintf(stderr, "Cannot open file %s\n", config->file);
            exit(-1);
        }
        if (fgets(line, MAXLINESIZE, config->tsfile)) {
            config->n = atoi(line);
            if (config->n == 0){
                fprintf(stderr, "Wrong file format\n");
                exit(-1);
            }
        }
    }
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, 0);
    DisplayParameters(config);
    gettimeofday(&gsBeginTime, NULL);
    PrintTime(gsBeginTime);
    fprintf(stdout, "emulation begins\n");;
    pthread_create(&thr, 0, monitor, 0);
    pthread_create(&thread_s1, 0, PacketServing, (void *)1);
    pthread_create(&thread_s2, 0, PacketServing, (void *)2);
    pthread_create(&thread_b, 0, TokenDepositing, (void *)config);
    pthread_create(&thread_p, 0, PacketArrival, (void *)config);
    pthread_join(thread_b, 0);
    pthread_join(thread_p, 0);
    pthread_join(thread_s1, 0);
    pthread_join(thread_s2, 0);
    pthread_kill(thr, SIGINT);
    pthread_join(thr, 0);
    Cleanup(&Q1);
    Cleanup(&Q2);
    gettimeofday(&gsEndTime, NULL);
    PrintTime(gsEndTime);
    fprintf(stdout, "emulation ends\n\n");;
    DisplayStat(stat);
}


void *monitor(void *arg)
{
    int sig;
    sigwait(&set, &sig);
    pthread_mutex_lock(&gsMutex);
    pthread_cancel(thread_b);
    pthread_cancel(thread_p);
    pthread_cancel(thread_s1);
    pthread_cancel(thread_s2);
    pthread_cond_broadcast(&gsCV);
    pthread_mutex_unlock(&gsMutex);
    return 0;
}


void DisplayParameters(Config *config)
{
    fprintf(stdout, "Emulation Parameters:\n");
    fprintf(stdout, "\tnumber to arrive = %d\n", config->n);
    if (config->r < 0.1)
        config->r = 0.1;
    fprintf(stdout, "\tr = %lf\n", config->r);
    fprintf(stdout, "\tB = %d\n", config->B);
    if (config->mode == TRACEDRIVEN) {
        fprintf(stdout, "\ttsfile = %s\n", config->file);
    } else {
        if (config->lambda < 0.1)
            config->lambda = 0.1;
        fprintf(stdout, "\tlambda = %lf\n", config->lambda);
        if (config->mu < 0.1)
            config->mu = 0.1;
        fprintf(stdout, "\tmu = %lf\n", config->mu);
        fprintf(stdout, "\tP = %d\n", config->P);
    }
    fprintf(stdout, "\n");
}


void DisplayStat(Statistics stat)
{
    long system_time;
    double std_devi;
    int total_token, total_packets;
    system_time = TimeGap(gsBeginTime, gsEndTime);
    fprintf(stdout, "Statistics:\n\n");
    fprintf(stdout, "\taverage packet inter-arrival time = %.6g\n",
            stat.avg_inter_arrival / 1000000);
    fprintf(stdout, "\taverage packet service time = %.6g\n\n",
            stat.avg_serve_time / 1000000);
    fprintf(stdout, "\taverage number of packets in Q1 = %.6g\n",
            stat.time_in_Q1 * 1000000 / system_time);
    fprintf(stdout, "\taverage number of packets in Q2 = %.6g\n",
            stat.time_in_Q2 * 1000000 / system_time);
    fprintf(stdout, "\taverage number of packets in S1 = %.6g\n",
            stat.time_in_S1 * 1000000 / system_time);
    fprintf(stdout, "\taverage number of packets in S2 = %.6g\n\n",
            stat.time_in_S2 * 1000000 / system_time);
    fprintf(stdout, "\taverage time a packet spent in system  = %.6g\n",
            stat.avg_time_in_system);
    std_devi = sqrt(stat.avg_time_square - 
                    stat.avg_time_in_system * stat.avg_time_in_system);
    fprintf(stdout, "\tstandard deviation for time spent in system = %.6g\n\n",
            std_devi);
    total_token = stat.dropped_token + stat.token;
    fprintf(stdout, "\ttoken drop probability = ");
    if (total_token == 0) {
        fprintf(stdout, "N/A (No tokens this time)\n");
    } else {
        fprintf(stdout, "%.6g\n", (double)stat.dropped_token / total_token);
    }
    total_packets = stat.dropped_packets + stat.completed_packets
                  + stat.removed_packets;
    fprintf(stdout, "\tpacket drop probability = ");
    if (total_packets == 0) {
        fprintf(stdout, "N/A (No packets this time)\n");
    } else {
        fprintf(stdout, "%.6g\n", (double)stat.dropped_packets / total_packets);
    }
}


void Cleanup(My402List *pList)
{
    My402ListElem *pElem = NULL;
    Packet *pPacket = NULL;
    int index, total_packets;
    if (pList == &Q1)
        index = 1;
    else
        index = 2;
    while (My402ListEmpty(pList) != TRUE) {
        pElem = My402ListFirst(pList);
        My402ListUnlink(pList, pElem);
        pPacket = (Packet *)pElem->obj;
        total_packets = stat.completed_packets + stat.dropped_packets
                      + stat.removed_packets;
        stat.avg_inter_arrival = (stat.avg_inter_arrival * total_packets
                               + pPacket->inter_arrival)
                               / (total_packets + 1);
        gettimeofday(&pPacket->timeline[4], NULL);
        PrintTime(pPacket->timeline[4]);
        fprintf(stdout, "p%d removed Q%d\n", pPacket->packet_no, index);
        stat.removed_packets += 1;
        free(pPacket);
        free(pElem);
    }
}
/* ------------------- Threads ---------------------------*/
void *PacketArrival(void *arg)
{
    /* Variable definition and init */
    int i, token_num;
    long inter_arrival, serve_time;
    char line[MAXLINESIZE];
    struct timeval last_arrival;
    Config *config;
    Packet *pPacket = NULL;
    config = (Config *)arg;
    token_num = config->P;
    inter_arrival = 1000 / config->lambda;
    serve_time = 1000 / config->mu;
    last_arrival = gsBeginTime;
    /* ----------- main thread logic ----------------*/
    for (i = 0; i < config->n; i ++) {
        if (config->mode == TRACEDRIVEN) {
            GetLine(config->tsfile, line);
            ParseLine(line, &inter_arrival, &token_num, &serve_time);
        }
        Sleep(last_arrival, inter_arrival);
        pthread_mutex_lock(&gsMutex);
        pthread_cleanup_push(UnlockMutex, &gsMutex);
        pPacket = CreatePacket(i+1, &last_arrival, token_num, serve_time, config->B);
        if (pPacket != NULL)
            Enqueue(&Q1, pPacket);
        if (IsEnoughToken() == TRUE) {
            pPacket = Dequeue(&Q1);
            if (My402ListEmpty(&Q2) == TRUE)
                pthread_cond_broadcast(&gsCV);
            Enqueue(&Q2, pPacket);
        }
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&gsMutex);
    }
    gsFinish = TRUE;
    return (void *)0;
}


void *TokenDepositing(void *arg)
{
    struct timeval last_arrival;
    long inter_arrival;
    int index = 1;
    Packet *pPacket;
    Config *config = (Config *)arg;
    last_arrival = gsBeginTime;
    inter_arrival = 1000 / config->r;
    while (!My402ListEmpty(&Q1) || !gsFinish ) {
        Sleep(last_arrival, inter_arrival);
        pthread_mutex_lock(&gsMutex);
        pthread_cleanup_push(UnlockMutex, &gsMutex);
        last_arrival = IncreaseToken(index, config->B);
        if (IsEnoughToken() == TRUE) {
            pPacket = Dequeue(&Q1);
            if (My402ListEmpty(&Q2) == TRUE)
                pthread_cond_broadcast(&gsCV);
            Enqueue(&Q2, pPacket);
        }
        pthread_cleanup_pop(0);
        pthread_mutex_unlock(&gsMutex);
        index ++;
    }
    pthread_cond_broadcast(&gsCV);
    return (void *)0;
}


void *PacketServing(void *arg)
{
    int server;
    Packet *pPacket = NULL;
    server = (int)arg;
    while (!gsFinish || !My402ListEmpty(&Q1) || !My402ListEmpty(&Q2)) {
        pthread_mutex_lock(&gsMutex);
        pthread_cleanup_push(UnlockMutex, &gsMutex);
        while (My402ListEmpty(&Q2) == TRUE
               && (!gsFinish || !My402ListEmpty(&Q1))) {
            pthread_cond_wait(&gsCV, &gsMutex);
        }
        if (My402ListEmpty(&Q2)) {
            pthread_mutex_unlock(&gsMutex);
            break;
        }
        pPacket = Dequeue(&Q2);
        pthread_cleanup_pop(1);
        pthread_mutex_unlock(&gsMutex);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        Serve(pPacket, server);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return (void *)0;
}


/* ----------------- Functions----------------------- */
void GetLine(FILE *fp, char line[])
{
    if (fgets(line, MAXLINESIZE, fp)) {
        if(line[strlen(line)-1] != '\n') {
            fprintf(stderr, "Line too long\n");
            exit(-1);
        }
    }
}


void ParseLine(char *s, long *inter_arrival,
               int *token_num, long *serve_time)
{
    char *index[3];
    int i;
    index[0] = s;
    for (i=0; i<2; i++) {
        index[i+1] = SeparateLine(index[i]);
        if (index[i+1] == NULL) {
            fprintf(stderr, "Malformed line\n");
            exit(-1);
        }
    }
    *inter_arrival = atoi(index[0]);
    *token_num = atoi(index[1]);
    *serve_time = atoi(index[2]);
}


char *SeparateLine(char *buf){
    char *pString = buf;
    while (*pString <= '9' && *pString >= '0') {
        pString++;
    } 
    while (*pString == ' ' || *pString == '\t') {
        pString++;
    } 
    return  pString;
}


void AfterComplete(Packet *pPacket, int server)
{
    int total_packets;
    total_packets = stat.completed_packets + stat.dropped_packets + stat.removed_packets;
    stat.avg_inter_arrival = 
        (stat.avg_inter_arrival * total_packets + pPacket->inter_arrival)
        / (total_packets + 1);
    stat.avg_serve_time = 
        (stat.avg_serve_time * stat.completed_packets + pPacket->serve_time)
        / (stat.completed_packets + 1);
    stat.avg_time_in_system =
        (stat.avg_time_in_system * stat.completed_packets
         + (float)pPacket->time_in_system / 1000000)
        / (stat.completed_packets + 1);
    stat.avg_time_square =
        (stat.avg_time_square * stat.completed_packets
        + pow((float)pPacket->time_in_system / 1000000, 2))
        / (stat.completed_packets + 1);
    stat.completed_packets += 1;
    stat.time_in_Q1 += (float)pPacket->time_in_Q1 / 1000000;
    stat.time_in_Q2 += (float)pPacket->time_in_Q2 / 1000000;
    if (server == 1)
        stat.time_in_S1 += (float)pPacket->serve_time / 1000000;
    else
        stat.time_in_S2 += (float)pPacket->serve_time / 1000000;
}


void Sleep(struct timeval last_arrival, long inter_arrival)
{
    struct timespec sleep_time, rem;
    struct timeval now;
    long buffer;
    gettimeofday(&now, NULL);
    buffer = TimeGap(last_arrival, now);
    buffer = inter_arrival*1000 - buffer;
    if (buffer > 0) {
        sleep_time.tv_sec = buffer / 1000000;
        sleep_time.tv_nsec = (buffer % 1000000) * 1000;
    } else {
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 0;
    }
    nanosleep(&sleep_time, &rem);
}


Packet *CreatePacket(int index, struct timeval *last_arrival,
                     int token_num, long serve_time, int bucket_depth)
{
    Packet *pPacket = NULL;
    int total_packets;
    pPacket = (Packet *)malloc(sizeof(Packet));
    pPacket->packet_no = index;
    pPacket->token_num = token_num;
    pPacket->serve_time = serve_time;
    gettimeofday(&pPacket->timeline[0], NULL);
    pPacket->inter_arrival = TimeGap(*last_arrival, pPacket->timeline[0]);
    *last_arrival = pPacket->timeline[0];
    PrintTime(pPacket->timeline[0]);
    fprintf(stdout,"p%d arrives, needs %d tokens, inter-arrival time = %.3fms",
            pPacket->packet_no,
            pPacket->token_num,
            (float)pPacket->inter_arrival/1000);
    if (pPacket->token_num > bucket_depth) {
        total_packets = stat.completed_packets + stat.dropped_packets
                      + stat.removed_packets;
        stat.avg_inter_arrival = 
            (stat.avg_inter_arrival * total_packets + pPacket->inter_arrival)
            / (total_packets + 1);
        stat.dropped_packets += 1;
        free(pPacket);
        fprintf(stdout, ", dropped\n");
        pPacket = NULL;
    } else {
        fprintf(stdout, "\n");
    }
    return pPacket;
}


struct timeval IncreaseToken(int index, int bucket_depth)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    PrintTime(t);
    if (gsTokenBucket < bucket_depth){
        gsTokenBucket ++;
        stat.token ++;
        fprintf(stdout, "token t%d arrives, token bucket now has %d tokens\n",
                index, gsTokenBucket);
    } else {
        stat.dropped_token ++;
        fprintf(stdout, "token t%d arrives, dropped\n", index);
    }
    return t;
}


int IsEnoughToken()
{
    My402ListElem *pElem = NULL;
    Packet *pPacket = NULL;
    if (My402ListEmpty(&Q1) != TRUE){
        pElem = My402ListFirst(&Q1);
        pPacket = (Packet *)(pElem->obj);
        if (pPacket->token_num <= gsTokenBucket)
            return TRUE;
        else
            return FALSE;
    }
    return FALSE;
}



long TimeGap(struct timeval last, struct timeval now)
{
    long sec, micro_sec;
    sec = now.tv_sec - last.tv_sec;
    micro_sec = sec * 1000000 + now.tv_usec - last.tv_usec;
    return micro_sec;
}


void Enqueue(My402List *pList, Packet *pPacket)
{
    My402ListAppend(pList, pPacket);
    if (pList == &Q1) {
        gettimeofday(&pPacket->timeline[1], NULL);
        PrintTime(pPacket->timeline[1]);
        fprintf(stdout, "p%d enters Q1\n", pPacket->packet_no);
    } else {
        gettimeofday(&pPacket->timeline[3], NULL);
        PrintTime(pPacket->timeline[3]);
        fprintf(stdout, "p%d enters Q2\n", pPacket->packet_no);
    }
}


Packet *Dequeue(My402List *pList)
{
    My402ListElem *pElem = NULL;
    Packet *pPacket = NULL;
    if (My402ListEmpty(pList) == TRUE)
        return FALSE;
    pElem = My402ListFirst(pList);
    My402ListUnlink(pList, pElem);
    pPacket = (Packet *)(pElem->obj);
    free(pElem);
    if (pList == &Q1) {
        gsTokenBucket -= pPacket->token_num;
        gettimeofday(&pPacket->timeline[2], NULL);
        pPacket->time_in_Q1 = TimeGap(pPacket->timeline[1], pPacket->timeline[2]);
        PrintTime(pPacket->timeline[2]);
        fprintf(stdout,
                "p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n",
                pPacket->packet_no,
                (float)pPacket->time_in_Q1 / 1000,
                gsTokenBucket);
    } else {
        gettimeofday(&pPacket->timeline[4], NULL);
        pPacket->time_in_Q2 = TimeGap(pPacket->timeline[3], pPacket->timeline[4]);
        PrintTime(pPacket->timeline[4]);
        fprintf(stdout, "p%d leaves Q2, time in Q2 = %.3fms\n",
                pPacket->packet_no,
                (float)pPacket->time_in_Q2 / 1000);
    }
    return pPacket;
}


void Serve(Packet *pPacket, int server)
{
    struct timespec serve_time, rem;
    serve_time.tv_sec = pPacket->serve_time / 1000;
    serve_time.tv_nsec = (pPacket->serve_time % 1000) * 1000000;
    pthread_mutex_lock(&gsMutex);
    gettimeofday(&pPacket->timeline[5], NULL);
    PrintTime(pPacket->timeline[5]);
    fprintf(stdout, "p%d begins service at S%d, requesting %ldms of service\n",
            pPacket->packet_no, server, pPacket->serve_time);
    pthread_mutex_unlock(&gsMutex);
    nanosleep(&serve_time, &rem);
    pthread_mutex_lock(&gsMutex);
    gettimeofday(&pPacket->timeline[6], NULL);
    pPacket->serve_time = TimeGap(pPacket->timeline[5], pPacket->timeline[6]);
    pPacket->time_in_system = TimeGap(pPacket->timeline[0], pPacket->timeline[6]);
    PrintTime(pPacket->timeline[6]);
    fprintf(stdout, "p%d departs from S%d, service time = %.3fms, time in system = %.3fms\n",
            pPacket->packet_no, server,
            (float)pPacket->serve_time / 1000,
            (float)pPacket->time_in_system / 1000);
    AfterComplete(pPacket, server);
    pthread_mutex_unlock(&gsMutex);
    free(pPacket);
}


void PrintTime(struct timeval t)
{
    long gap;
    gap = TimeGap(gsBeginTime, t);
    fprintf(stdout, "%012.3fms: ", (float)gap/1000);
}


void UnlockMutex(void *mutex)
{
    pthread_mutex_unlock(mutex);
}


int main(int argc, char *argv[])
{
    Config config;
    SetProgramName(*argv);
    config = ProcessOptions(argc, argv);

    Process(&config);
    return 0;
}
