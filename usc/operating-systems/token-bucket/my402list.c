#include <stdio.h>
#include <stdlib.h>
#include "my402list.h"


int My402ListInit(My402List *pList){
    pList->num_members = 0;
    if(pList!=NULL){
        pList->anchor.next = &(pList->anchor);
        pList->anchor.prev = &(pList->anchor);
        pList->anchor.obj = NULL;
    }else{
        return FALSE;
    }
    return TRUE;
}

My402ListElem *My402ListFirst(My402List *pList){
    if(My402ListEmpty(pList)==TRUE)
        return NULL;
    else
        return pList->anchor.next;
}


My402ListElem *My402ListLast(My402List *pList){
    if(My402ListEmpty(pList)==TRUE)
        return NULL;
    else
        return pList->anchor.prev;
}


int My402ListAppend(My402List *pList, void *obj){
    My402ListElem *pElem, *pLast = NULL;

    pElem = (My402ListElem *)malloc(sizeof(My402ListElem));
    if(pElem==NULL)
        return FALSE;
    pElem->obj = obj;

    if(My402ListEmpty(pList)==TRUE){
        pList->anchor.next = pElem;
        pList->anchor.prev = pElem;
        pElem->next = &(pList->anchor);
        pElem->prev = &(pList->anchor);
    }else{
        pLast = My402ListLast(pList);
        pLast->next = pElem;
        pList->anchor.prev = pElem;
        pElem->next = &(pList->anchor);
        pElem->prev = pLast;
    }
    pList->num_members++;
    return TRUE;
}


int My402ListPrepend(My402List *pList, void *obj){
    My402ListElem *pElem, *pFirst = NULL;

    pElem = (My402ListElem *)malloc(sizeof(My402ListElem));
    pElem->obj = obj;
    if(My402ListEmpty(pList)==TRUE){
        pList->anchor.next = pElem;
        pList->anchor.prev = pElem;
        pElem->next = &(pList->anchor);
        pElem->prev = &(pList->anchor);
    }else{
        pFirst = My402ListFirst(pList);
        pFirst->prev = pElem;
        pList->anchor.next = pElem;
        pElem->prev = &(pList->anchor);
        pElem->next = pFirst;
    }
    pList->num_members++;
    return TRUE;
}


void My402ListUnlink(My402List *pList, My402ListElem *pElem){
    My402ListElem *pNext, *pPrev, *pLast, *pFirst = NULL;

    pFirst = My402ListFirst(pList);
    pLast = My402ListLast(pList);

    if(pElem == pFirst)
        pPrev = &(pList->anchor);
    else
        pPrev = My402ListPrev(pList, pElem);
    if(pElem == pLast)
        pNext = &(pList->anchor);
    else
        pNext = My402ListNext(pList, pElem);
    pPrev->next = pNext;
    pNext->prev = pPrev;
    pElem->next = NULL;
    pElem->prev = NULL;
    pList->num_members--;
}


void My402ListUnlinkAll(My402List *pList){
    My402ListElem *pFirst = NULL;

    pFirst = My402ListFirst(pList);
    while(pFirst!=NULL){
        My402ListUnlink(pList, pFirst);
        pFirst = My402ListFirst(pList);
    }
    pList->num_members = 0;
}


int My402ListInsertBefore(My402List *pList, void *obj, My402ListElem *pElem){
    My402ListElem *pNew, *pPrev, *pFirst= NULL;

    pNew = (My402ListElem *)malloc(sizeof(My402ListElem));
    pFirst = My402ListFirst(pList);

    if(pElem==pFirst||pElem==NULL){
        My402ListPrepend(pList, obj);
    }else{
        pNew->obj = obj;
        pPrev = My402ListPrev(pList, pElem);
        pPrev->next = pNew;
        pNew->prev = pPrev;
        pElem->prev = pNew;
        pNew->next = pElem;
        pList->num_members++;
    }
    return TRUE;
}


int My402ListInsertAfter(My402List *pList, void *obj, My402ListElem *pElem){
    My402ListElem *pNew, *pNext, *pLast = NULL;

    pNew = (My402ListElem *)malloc(sizeof(My402ListElem));
    pLast = My402ListLast(pList);

    if(pElem==pLast||pElem==NULL){
        My402ListAppend(pList, obj);
    }else{
        pNew->obj = obj;
        pNext = My402ListNext(pList, pElem);
        pNext->prev = pNew;
        pNew->next = pNext;
        pElem->next = pNew;
        pNew->prev = pElem;
        pList->num_members++;
    }
    return TRUE;
}


My402ListElem *My402ListNext(My402List *pList, My402ListElem *pElem){
    if(My402ListLast(pList)==pElem)
        return NULL;
    else
        return pElem->next;
}


My402ListElem *My402ListPrev(My402List *pList, My402ListElem *pElem){
    if(My402ListFirst(pList)==pElem)
        return NULL;
    else
        return pElem->prev;
}


My402ListElem *My402ListFind(My402List *pList, void *obj){
    My402ListElem *pTarget = NULL;
    My402ListElem *pElem = NULL;

    if(My402ListEmpty(pList)==TRUE){
        return NULL;
    }
    for(pElem=My402ListFirst(pList);pElem!=NULL;pElem=My402ListNext(pList, pElem)){
        if(pElem->obj==obj){
            pTarget = pElem;
            break;
        }
    }
    return pTarget;
}


int My402ListEmpty(My402List *pList){
    if(pList->anchor.next==&(pList->anchor))
        return TRUE;
    else
        return FALSE;
}


int My402ListLength(My402List *pList){
    return pList->num_members;
}


void Traverse(My402List *list){
    My402ListElem *elem=NULL;
    int *integer;

    for(elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
        integer = (int *)elem->obj;
        printf("%d\n",*integer);
        /* access foo here */
    }
}
