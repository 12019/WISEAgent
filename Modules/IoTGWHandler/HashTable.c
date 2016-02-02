/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Array Library Def          			    										             */
/* Reference    : None																									 */
/****************************************************************************/
#include "platform.h"
#include "ArrayLib.h"
#include "HashTable.h"
#include <limits.h>

void HashTableTest() {
  printf("\n=======HashTableTest()==========\n");
  char *names[] = { "John", "Mary", "George", "Mickey", "Snoopy", "Bob", "Jack" };
  char *ids[]    = { "1",    "2",    "3",      "4",      "5",      "6",   "7" };
  HashTable* table = HashTableNew(3);
  int i;
  for (i=0; i<5; i++)
    HashTablePut(table, names[i], ids[i]);
//  for (i=0; i<7; i++)
//    printf("id=%s\n", HashTableGet(table, names[i]));
//  HashTableEach(table, strPrintln);
  HashTableFree(table);
}

int hash(  char *key, int range ) {
 
	unsigned long int hashval = 0;
	int i = 0;
 
	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}
 
	return hashval %range;
}

int hash2(char *key, int range) {
  int i, hashCode=0;
  for (i=0; i<strlen(key); i++) {
    BYTE value = (BYTE) key[i];
    hashCode += value;
    hashCode %= range;
  }
  return hashCode;
}

Entry* EntryNew(char *key, void *data) {
  Entry* e = (Entry*)ObjNew(Entry, 1);
  memset(e->key,0,sizeof(e->key));
  //e->key = key;
  snprintf(e->key,sizeof(e->key),"%s",key);
  e->data = data;
  return e;
}

void EntryFree(Entry *e) {
  ObjFree(e);
}

int EntryCompare(Entry *e1, Entry *e2) {
  return strcmp(e1->key, e2->key);
}

HashTable* HashTableNew(int size) {
  Array *table = ArrayNew(size);
  int i;
  for (i=0; i<table->size; i++)
    ArrayAdd(table, ArrayNew(1));
  return table;
}

void HashTableFree(HashTable *table) {
  int ti, hi;
  for (ti=0; ti<table->size; ti++) {
    Array *hitArray = (Array *)table->item[ti];
    ArrayFree(hitArray, (FuncPtr1) EntryFree);
  }
  ArrayFree(table, NULL);
}


// 尋找雜湊表中 key 所對應的元素並傳回
void *HashTableGet(HashTable *table, char *key) { 
  Entry keyEntry;
  memset(&keyEntry,0,sizeof(keyEntry));

  int slot = hash(key, table->size);            // 取得雜湊值 (列號) 

  Array *hitArray = (Array*) table->item[slot]; // 取得該列
  // 找出該列中是否有包含 key 的配對
  //keyEntry.key = key;
  snprintf(keyEntry.key,sizeof(keyEntry.key),"%s",key);

  int keyIdx = ArrayFind(hitArray, &keyEntry, (FuncPtr2)EntryCompare);
  if (keyIdx >= 0) { // 若有，則傳回該配對的資料元素 
    Entry *e = (Entry *)hitArray->item[keyIdx];
    return e->data;
  }
  return NULL; // 否則傳回 NULL 
}

// 將 (key, data) 配對放入雜湊表中 
void HashTablePut(HashTable *table, char *key, void *data) {
  Entry *e;
  Entry keyEntry;
  memset(&keyEntry,0,sizeof(keyEntry));
  int slot = hash(key, table->size);            // 取得雜湊值 (列號) 

  Array *hitArray = (Array*) table->item[slot]; // 取得該列
  //keyEntry.key = key;
  snprintf(keyEntry.key,sizeof(keyEntry.key),"%s",key);

  int keyIdx = ArrayFind(hitArray, &keyEntry, (FuncPtr2)EntryCompare);
  if (keyIdx >= 0) { // 若有，則傳回該配對的資料元素 
    e = (Entry *)hitArray->item[keyIdx];
    e->data = data;
  } else {
    e= EntryNew(key, data);// 建立配對 
    ArrayAdd(hitArray, e); // 放入對應的列中 
  }
}

void HashTableEach(HashTable *table, FuncPtr1 f) {
  int i, j;
  for (i=0; i<table->count; i++) {
    Array *hits = (Array *)table->item[i];
    for (j=0; j<hits->count; j++) {
      Entry *e = (Entry *)hits->item[j];
      f(e->data);
    }
  }
}

Array* HashTableToArray(HashTable *table) {
  Array *array = ArrayNew(table->count);
  int i, j;
  for (i=0; i<table->count; i++) {
    Array *hits = (Array *)table->item[i];
    for (j=0; j<hits->count; j++) {
      Entry *e = (Entry *)hits->item[j];
      ArrayAdd(array, e->data);
    }
  }
  return array;
}