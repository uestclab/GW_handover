#include "hash_tbl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MEM_ERR  "Fail to allocate memory"

void err_exit(const char *msg){
    printf("%s\n", msg);
    exit(1);
}

#define HT_BASE 3

static hash_node *new_hash_node(const char *k, const void *v, int value_size){
    hash_node *n = malloc(sizeof(hash_node));
    if(!n)
        err_exit(MEM_ERR);
    n->key = strdup(k);
    //n->val = strdup(v);

    n->val = malloc(value_size);
    n->val = memcpy(n->val,v,value_size);


    n->next = NULL;
    return n;
}

static void del_hash_node(hash_node **node_ptr){
    free((*node_ptr)->key);
    free((*node_ptr)->val);
    free(*node_ptr);
    *node_ptr = NULL;
}

static int hash_func(const char *s, int base, int bucket_num){
    long hash = 0;
    int len = strlen(s);
    for(int i = 0; i < len; i++){
        hash += (long)pow(base, len - (i + 1))*s[i];
        hash = hash%bucket_num;
    }
    return (int)hash;
}

void init_hash_tbl(hash_tbl *tbl){
    tbl->bucket_num = INIT_BUCKET_NUM;
    tbl->node_num = 0;
    tbl->buckets = calloc(tbl->bucket_num, sizeof(hash_node*));
    if(!tbl->buckets)
        err_exit(MEM_ERR);
}

void insert_hash_tbl(hash_tbl *tbl, const char *k,
    const void *v, int value_size){
    hash_node *item = new_hash_node(k, v, value_size);
    int ix = hash_func(k, HT_BASE, tbl->bucket_num);
    
    if(!tbl->buckets[ix]){
        tbl->buckets[ix] = item;
    }else{
        hash_node dummy = {NULL, NULL, NULL};
        dummy.next = tbl->buckets[ix];
        hash_node *prev = &dummy;

        while(prev && prev->next){
            //update node if key exists
            if(prev->next->key && 
              strcmp(prev->next->key, k) == 0){
                item->next = prev->next->next;
                del_hash_node(&(prev->next));
                prev->next = item;
                return;
            }
            prev = prev->next;
        }
        //link the new node
        prev->next = item;
    }
    tbl->node_num++;
}

void *search_hash_tbl(hash_tbl *tbl, const char *k){
    int ix = hash_func(k, HT_BASE, tbl->bucket_num);
    hash_node *prev = tbl->buckets[ix];
    while(prev){
        if(prev->key && strcmp(prev->key, k) == 0){
            return prev->val;
        }
        prev = prev->next;
    }
    return NULL;
}

void reset_hash_tbl(hash_tbl *tbl){
    for(int i = 0; i < tbl->bucket_num; i++){
        if(tbl->buckets[i]){
            del_hash_node(&(tbl->buckets[i]));
        }
    }
    free(tbl->buckets);
    tbl->buckets = NULL;
    tbl->bucket_num = 0;
    tbl->node_num = 0;
}

void del_key_hash_tbl(hash_tbl *tbl, const char *k){
    int ix = hash_func(k, HT_BASE, tbl->bucket_num);
    hash_node dummy = {NULL, NULL, NULL};
    dummy.next = tbl->buckets[ix];
    hash_node *prev = &dummy;
    while(prev && prev->next){
        if(prev->next->key && 
           strcmp(prev->next->key, k) == 0){
            hash_node *next = prev->next->next;
            del_hash_node(&(prev->next));
            prev->next = next;
            tbl->node_num--;
            break;
        }
        prev = prev->next;
    }
    tbl->buckets[ix] = dummy.next;
}

void test_char_hash_tbl(){
    printf("test (key,value) : (char*,char) \n");
    hash_tbl tbl;
    init_hash_tbl(&tbl);
    char key[125], val[125];
    for(char c = 'a'; c <= 'z'; c++){
        sprintf(key, "%c", c);
        sprintf(val, "%c", c + 'A' - 'a');
        //printf("insert %s-%s\n", key, val);
        insert_hash_tbl(&tbl, key, val, 2);
    }

    for(char c = 'f'; c <= 'l'; c++){
        sprintf(key, "%c", c);
        if(search_hash_tbl(&tbl, key)){
            sprintf(val, "%s", search_hash_tbl(&tbl, key));
            printf("%s-%s\n", key, val);
        }
        del_key_hash_tbl(&tbl, key);
    }

    reset_hash_tbl(&tbl);
}

void test_int_hash_tbl(){
    printf("test (key,value) : (char*,int) \n");
    hash_tbl tbl;
    init_hash_tbl(&tbl);
    char key[125];
    int val;
    for(char c = 'a'; c <= 'z'; c++){
        sprintf(key, "%c", c);
        val = c + 'A' - 'a';
        //printf("insert %s-%s\n", key, val);
        insert_hash_tbl(&tbl, key, (void*)(&val), sizeof(int));
    }

    for(char c = 'f'; c <= 'h'; c++){
        sprintf(key, "%c", c);
        if(search_hash_tbl(&tbl, key)){
            //sprintf(val, "%s", search_hash_tbl(&tbl, key));
            int* tmp_val = (int*)search_hash_tbl(&tbl,key);
            printf("%s-%d\n", key, *tmp_val);
        }
        del_key_hash_tbl(&tbl, key);
    }

    reset_hash_tbl(&tbl);
}