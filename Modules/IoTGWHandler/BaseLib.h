/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Basic Library definition			    										             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __BASE_LIE_H__
#define __BASE_LIE_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stddef.h>

#define FALSE							0
#define TRUE							1
#define BYTE      unsigned char
#define BOOL    unsigned char
#define INT32					     int
#define INT16   short
#define INT8    char
#define UINT32  unsigned int
#define UINT16  unsigned short
#define UINT8  unsigned char
#define MAX_LEN 512


#define min(x,y)         (x < y?x:y)
#define max(x,y)         (x > y?x:y)
#define ASSERT(cond)     assert(cond)
#define ObjNew(type, count) newMemory(count*sizeof(type))
#define ObjFree freeMemory
#define strFree freeMemory

void* newMemory(int size);
void freeMemory(void *ptr);
void checkMemory();
char *newStr(char *str);
char *newSubstr(char *str, int i, int len);
BYTE* newFileBytes(char *fileName, int *sizePtr);
char* newFileStr(char *fileName);


#define strEqual(str1, str2) (strcmp(str1, str2)==0)
#define strMember(ch, set) (strchr(set, ch) != NULL)
void strSubstr(char *substr, char *str, int i, int len);
void strReplace(char *str, char *from, char to);
void strTrim(char *trimStr, char *str, char *set);
char *strSpaces(int len);
void strToUpper(char *str);
BOOL strPartOf(char *token, char *set);
void strPrint(void *data);
void strPrintln(void *data);
char *repl_str(const char *str, const char *old, const char *newstr);

typedef void (*FuncPtr1) (void *);
typedef int (*FuncPtr2) (void *, void *);

extern char SPLITER[];
extern char SPACE[];
extern char ALPHA[];
extern char DIGIT[];

#endif // __BASE_LIE_H__