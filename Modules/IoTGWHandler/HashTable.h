/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Array Library Def          			    										             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __HASHTABLE_H__
#define __HASHTABLE_H__

#include "ArrayLib.h"

typedef struct {
  char  key[80];
  void *data;
} Entry;

Entry* EntryNew(char *key, void *data);
int EntryCompare(Entry *e1, Entry *e2);

int hash(char *key, int range);

#define HashTable Array

HashTable* HashTableNew(int size);
void HashTableFree(HashTable *table);
void HashTablePut(HashTable *table, char *key, void *data);
void *HashTableGet(HashTable *table, char *key);
void HashTableEach(HashTable *table, FuncPtr1 f);
Array* HashTableToArray(HashTable *table);
void HashTableTest();

#endif //__HASHTABLE_H__