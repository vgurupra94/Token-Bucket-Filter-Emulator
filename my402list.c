#ifndef _MY402LIST_C_
#define _MY402LIST_C_

#include "my402list.h"
#include "stdlib.h"
#include "stdio.h"
#include "cs402.h"


int  My402ListLength(My402List *MyList){
    return MyList->num_members;
}

int  My402ListEmpty(My402List *elem){
    if(elem->num_members == 0)
        return 1;
    return 0;
}

int  My402ListAppend(My402List *list, void *obj){
    My402ListElem *node = NULL;
    My402List *temporary = NULL ;
    temporary = list;       
    
    if( My402ListEmpty(list)){
        
        node = malloc(sizeof( My402ListElem));
        if( node == 0){
            fprintf(stderr,"ERROR: Out of Memory");
            return 0;
        }

        temporary->num_members = 1;
        node->obj = obj;
        temporary->anchor.next = node;
        node->next = &(temporary->anchor);
        temporary->anchor.prev = node;
        node->prev = &(temporary->anchor);
        
    }
    else{
        node = malloc(sizeof(My402ListElem));
        
        if( node == 0){
            fprintf(stderr,"ERROR: Out of Memory");
            return FALSE;
        }
        
        temporary->num_members++;
        node->obj = obj;
        node->next = &(temporary->anchor);
        node->prev = temporary->anchor.prev;
        temporary->anchor.prev->next = node;
        temporary->anchor.prev = node;        
        
    }
    
    return TRUE;
    
}

int  My402ListPrepend(My402List *list, void *obj){
    
    My402List *temporary = NULL ;
    temporary = list;

    My402ListElem *r = NULL;   
    
    if( My402ListEmpty(list)){
        
        r = malloc(sizeof( My402ListElem));
        if( r == 0){
            fprintf(stderr,"ERROR: Out of Memory");
            return FALSE;
        }

         
        temporary->num_members = 1;
        r->obj = obj;
        temporary->anchor.next = r;
        r->next = &(temporary->anchor);
        temporary->anchor.prev = r;
        r->prev = &(temporary->anchor);
               
        
    }
    
    else{
        
        r = malloc(sizeof(My402ListElem));
        
        if( r == 0){
            fprintf(stderr,"ERROR: Out of Memory");
            return FALSE;
        }
        
        temporary->num_members++; 
        r->obj = obj;
        r->next = temporary->anchor.next;
        r->prev = &(temporary->anchor);
        temporary->anchor.next->prev = r;
        temporary->anchor.next = r;  
        
    }
    
    return TRUE;
}

void My402ListUnlink(My402List *list, My402ListElem *elem){
    
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;   
    list->num_members--;
    free(elem);
    
}

void My402ListUnlinkAll(My402List *list){
    My402ListElem *elem=NULL;
    
    for (elem=My402ListFirst(list);elem != NULL;elem=My402ListNext(list, elem)) {
        
        My402ListUnlink(list,elem);
        
    }
    
}

int  My402ListInsertAfter(My402List *list, void *obj, My402ListElem *elem){
    if(elem == NULL){
        My402ListAppend(list,obj);
    }
    else{
        My402ListElem *newNode = NULL;
        newNode = malloc(sizeof(My402ListElem));
        
        if(newNode == NULL){
            return FALSE;
        }
        
        newNode->obj = obj;
        newNode->next = elem->next;
        newNode->prev = elem;
        elem->next->prev = newNode;
        elem->next = newNode;  
        list->num_members++;     
        
    }
    
    return TRUE;
}

int  My402ListInsertBefore(My402List *list, void *obj, My402ListElem *elem){
    if(elem == NULL){
        My402ListPrepend(list,obj);
        
    }
    else{
        My402ListElem *newNode = NULL;
        newNode = malloc(sizeof(My402ListElem));
        
        if(newNode == NULL){
            return FALSE;
        }

        newNode->obj = obj;
        newNode->next = elem;
        newNode->prev = elem->prev;
        elem->prev->next = newNode;
        elem->prev = newNode;      
        list->num_members++;
        
    }
    
    return TRUE;
}

My402ListElem *My402ListFirst(My402List *list){
    
    if(My402ListLength(list) == 0 )
        return NULL;
    
    return list->anchor.next ;   
}

My402ListElem *My402ListLast(My402List *list){
    
    if(My402ListLength(list) == 0 )
        return NULL;
        
    return list->anchor.prev ;  
    
}

My402ListElem *My402ListNext(My402List *list, My402ListElem *elem){
    
    
    if(elem->next == &(list->anchor))
        return NULL;
    
    return elem->next;   
}

My402ListElem *My402ListPrev(My402List *list, My402ListElem *elem){
    
    if(elem->prev == &(list->anchor))
        return NULL;
        
    return elem->prev;   
}

My402ListElem *My402ListFind(My402List *list, void *obj){
    My402ListElem *elem=NULL;
    
    for (elem=My402ListFirst(list);elem != NULL;elem=My402ListNext(list, elem)) {
        
        if(elem->obj == obj){
            return elem;   
        }
    }
    return NULL;    
}

int My402ListInit(My402List* MyList) {
    if (!MyList) { 
        return FALSE;
    }
    MyList->anchor.obj = NULL;
    MyList->anchor.next = NULL;
    MyList->anchor.prev = NULL;
    MyList->num_members = 0;
    return TRUE;
}

#endif
