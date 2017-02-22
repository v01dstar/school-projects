#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "my402list.h"
#include "my402trans.h"

static int readFile = FALSE;
static char fileName[MAXPATHLENGTH];
static char gszProgName[MAXPATHLENGTH];


/* ----------------------- Utility Functions ----------------------- */

static void Usage(){
    fprintf(stderr, "usage: %s %s\n", gszProgName, "sort [tfile]");
    exit(-1);
}


static void ProcessOptions(int argc, char *argv[]){
    if(argc<=1){
        fprintf(stderr, "Malformed command\n");
        Usage();
    }
    for(argc--, argv++; argc>0; argc--, argv++){
        if(strcmp(*argv, "sort")==0){
            if(argc > 1){
                argc--;
                argv++;
                readFile = TRUE;
                strcpy(fileName, *argv);
            }
            Sort();
        }else{
            fprintf(stderr, "Malformed command\n");
            Usage();
        }
    }
}


static void SetProgramName(char *s){
    char *c_ptr=strrchr(s, DIR_SEP);

    if (c_ptr == NULL) {
        strcpy(gszProgName, s);
    } else {
        strcpy(gszProgName, ++c_ptr);
    }
}


/* -------------------- Read Sort and Show --------------- */
void Sort(){
    My402List list;

    memset(&list, 0, sizeof(My402List));
    My402ListInit(&list);
    My402TransLoad(&list);
    if(My402ListLength(&list)==0){
        fprintf(stderr,"Empty file.\n");
        exit(-1);
    }
    DisplayRecords(&list);
}


char *SeparateTab(char *buf){
    char *pTab = strchr(buf, '\t');
    if(pTab!=NULL)
        *pTab++ = '\0';
    else
        return NULL;
    return  pTab;
}


void ParseLine(char *line, My402Trans *pTrans){
    char *index[4], *integer, *decimal;
    int i, length, type=DEPOSIT;
    time_t now;

    integer = NULL;
    decimal = NULL;
    i = 0;
    length = 0;
    time(&now);
    /*Separate by tab*/
    index[0] = line;
    for(i=0;i<3;i++){
        index[i+1] = SeparateTab(index[i]);
        if(index[i+1]==NULL){
            fprintf(stderr,"Malformed line\n");
            exit(-1);
        }
    }
    if(SeparateTab(index[3])!=NULL){
        fprintf(stderr,"Too many fields\n");
        exit(-1); 
    }
    /*Type examination*/
    if(strcmp(index[0], "+")==0){
        type = DEPOSIT;
    }else if(strcmp(index[0], "-")==0){
        type = WITHDRAWAL;
    }else{
        fprintf(stderr,"Malformed type.\n");
        exit(-1);
    }
    /*Timestamp examination and transform*/
    if(strlen(index[1])<=10){
        pTrans->timestamp = atoi(index[1]);
        if(pTrans->timestamp>(int)now||pTrans->timestamp<0){
            fprintf(stderr,"Timestamp can not be true\n");
            exit(-1); 
        }
    }else{
        fprintf(stderr,"Malformed timestamp\n");
        exit(-1);
    }
    /*Amount examination and transform*/
    pTrans->amount = 0;
    integer = index[2];
    decimal = strchr(index[2], '.');
    if(decimal!=NULL)
        *decimal ++= '\0';
    if(strlen(integer)>7||strlen(decimal)>2){
        fprintf(stderr,"Malformed amount: digits error\n");
        exit(-1);        
    }
    length = strlen(integer);
    for(i=0;i<length;i++){
        if(index[2][i]>='0'&&index[2][i]<='9'){
            pTrans->amount = pTrans->amount*10 + (index[2][i] - '0');
        }else{
            fprintf(stderr,"Malformed amount: unknown character\n");
            exit(-1);
        }
    }
    pTrans->amount = pTrans->amount * 100;
    if(decimal!=NULL){
        if(decimal[0]>='0'&&decimal[0]<='9')
            pTrans->amount += (decimal[0] - '0') * 10;
        if(strlen(decimal)==2&&decimal[1]>='0'&&decimal[1]<='9')
            pTrans->amount += decimal[1] - '0';
    }
    if(type==WITHDRAWAL)
        pTrans->amount = -pTrans->amount;
    /*Description examination*/
    index[3] = Trim(index[3]);
    if(index[3]){
        pTrans->desc = (char *)malloc(strlen(index[3]));
        strncpy(pTrans->desc, index[3], strlen(index[3]));
    }else{
        fprintf(stderr,"Malformed description: empty\n");
        exit(-1);
    }
}



char *Trim(char *str){
    char *end;

    end = NULL;
    while(*str==' ') str++;
    if(*str=='\0')
        return NULL;
    end = str + strlen(str) - 1;
    while(end>str&&*end==' ') end--;
    *++end = '\0';
    return str;
}


int My402TransLoad(My402List *pList){
    FILE *inStream = NULL;
    My402Trans *pTrans = NULL;
    char line[MAXLINESIZE+1];
    int length=0;

    pTrans = (My402Trans *)malloc(sizeof(My402Trans));

    if(readFile>0){
        if((inStream = fopen(fileName, "r"))==NULL){
            fprintf(stderr,"Fail to open file\n");
            exit(-1);
        }
    }else{
        inStream = stdin;
    }
    while(fgets(line, MAXLINESIZE+1, inStream)){
        /*Overall examination*/
        length = strlen(line);
        if(line[length-1]!='\n'){
            fprintf(stderr,"Line is too long.\n");
            exit(-1);
        }else{
            line[length-1] = '\0';
        }
        ParseLine(line, pTrans);
        My402TransAdd(pList, pTrans);
        pTrans = NULL;
        pTrans = (My402Trans *)malloc(sizeof(My402Trans));
    }
    if(readFile>0)
        fclose(inStream);
    return TRUE;
}


int My402TransAdd(My402List *pList, My402Trans *pTrans){
    My402ListElem *pElem, *index = NULL;
    My402Trans *trans = NULL;

  
    for(pElem=My402ListFirst(pList); pElem!= NULL; pElem=My402ListNext(pList, pElem)) {
        trans = (My402Trans *)pElem->obj;
        if(pTrans->timestamp<trans->timestamp){
            index = pElem;
            break;
        }else if(pTrans->timestamp==trans->timestamp){
            fprintf(stderr, "Timestamp repeat.\n");
            exit(-1);
        }
    }
    if(index==NULL)
        My402ListAppend(pList, pTrans);
    else
        My402ListInsertBefore(pList, pTrans, index);
    return TRUE;
}


void DisplayRecords(My402List *pList){
    My402ListElem *pElem = NULL;
    My402Trans *pTrans = NULL;
    time_t t;
    char *timeString, *descString, *amountString, *balanceString;
    long long balance = 0;

    timeString = (char *)malloc(16);
    descString = (char *)malloc(25);
    amountString = (char *)malloc(13);
    balanceString = (char *)malloc(13);

    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    for(pElem=My402ListFirst(pList); pElem!= NULL; pElem=My402ListNext(pList, pElem)) {
        pTrans = (My402Trans *)pElem->obj;

        t = pTrans->timestamp;
        strftime(timeString, 16, "%a %b %e %Y", localtime(&t));
        if(strlen(pTrans->desc)>24){
            strncpy(descString, pTrans->desc, 24);
        }else{
            strncpy(descString, pTrans->desc, strlen(pTrans->desc));
        }
        balance += pTrans->amount;
        FormatMoney(amountString, pTrans->amount);
        FormatMoney(balanceString, balance);
        printf("| %-15s | %-24s ", timeString, descString); 
        if(pTrans->amount<0)
            printf("| (%12s) ", amountString);
        else
            printf("|  %12s  ", amountString);
        if(balance<0)
            printf("| (%12s) |\n", balanceString);
        else
            printf("|  %12s  |\n", balanceString);
        memset(timeString, 0, 16);
        memset(descString, 0, 24);
        memset(amountString, 0, 13);
        memset(balanceString, 0, 13);
    }
    printf("+-----------------+--------------------------+----------------+----------------+\n");
}


void FormatMoney(char *str, long long money){
    int decimal, integer, length, i, j=0;
    char buf[7];

    decimal = 0;
    integer = 0;
    length = 0;
    if(money<0){
        money = -money;
    }
    if(money>999999999){
        strcpy(str,"?,???,???.??");
    }else{
        decimal = money % 100;
        integer = money / 100;
        sprintf(buf, "%d", integer);
        length = strlen(buf);
        for(i=0;i<length;i++,j++){
            str[j] = buf[i];
            if((length-1-i)%3==0&&i!=length-1) 
                str[++j] = ',';
        }
        str[j++] = '.';
        str[j++] = decimal/10 + '0';
        str[j++] = decimal%10 + '0';
        str[j] = '\0';
    }
}



int main(int argc, char *argv[]){
    SetProgramName(*argv);
    ProcessOptions(argc, argv);

    return TRUE;
}
