/*
 * Author:   Yang Zhang  (hairedorange@gmail.com)
 *
 * Version:  0.1
 */

#include <time.h>
#include "cs402.h"
#include "my402list.h"


#ifndef MAXLINESIZE
#define MAXLINESIZE 1024
#endif

#ifndef DEPOSIT
#define DEPOSIT 0
#define WITHDRAWAL 1
#endif

typedef struct tagMy402Trans{
    int timestamp;
    int amount;
    char *desc;
}My402Trans;


extern void Sort();
extern int My402TransLoad(My402List*);
extern int My402TransAdd(My402List*, My402Trans*);
extern void ParseLine(char*, My402Trans*);
extern char *SeparateTab(char*);
extern char *Trim(char*);
extern void DisplayRecords(My402List*);
extern void FormatMoney(char*, long long);
