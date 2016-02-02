/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Base  Library Function             													    */
/* Reference    : None																									 */
/****************************************************************************/
#include <ctype.h>
#include "BaseLib.h"

char SPLITER[] = " \t\n\r~!@#$%^&*()_+{}:\"<>?-=[]|\\;',./";
char SPACE[] = " \t\n\r";
char ALPHA[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char DIGIT[] = "0123456789";
char NAME_CHAR[] = "_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";


int newMemoryCount = 0;

void* newMemory(int size) {
  void *ptr=malloc(size);
  assert(ptr != NULL);
  memset(ptr, 0, size);
//  printf("memGet:%p\n", ptr);
  newMemoryCount++;
  return ptr;
}

int freeMemoryCount=0;

void freeMemory(void *ptr) {
//  printf("memFree:%p\n", ptr);
  free(ptr);
  freeMemoryCount++;
}

void checkMemory() {
  printf("newMemoryCount=%d freeMemoryCount=%d\n", newMemoryCount, freeMemoryCount);
}

// 檔案輸出入 
BYTE* newFileBytes(char *fileName, int *sizePtr) {
  FILE *file = fopen(fileName, "rb");
  fseek(file, 0 , SEEK_END);
  long size = ftell(file);
  rewind(file);
  BYTE *buffer = (BYTE*) newMemory(size+1);
  fread (buffer,size,1,file);
  fclose(file);
  *sizePtr = size;
  return buffer;
}

char* newFileStr(char *fileName) {
  int size;
  BYTE *buffer = newFileBytes(fileName, &size);
  buffer[size] = '\0';
  return (char*) buffer;
}

char *newStr(char *str) {
  char *rzStr = (char *)newMemory(strlen(str)+1);
  strcpy(rzStr, str);
  return rzStr;
}

char *newSubstr(char *str, int i, int len) {
  char *rzStr = (char *)newMemory(len+1);
  strSubstr(rzStr, str, i, len);
  return rzStr;
}

// 字串函數 
void strPrint(void *data) {
  printf("%s ", data);
}

void strPrintln(void *data) {
  printf("%s\n", data);
}

BOOL strHead(char *str, char *head) { 
  return (strncmp(str, head, strlen(head))==0);
}

BOOL strTail(char *str, char *tail) {
  int strLen = strlen(str), tailLen = strlen(tail);
  if (strLen < tailLen) return FALSE;
  return (strcmp(str+strLen-tailLen, tail)==0);
}

int strCountChar(char *str, char *charSet) {
  int i, count=0;
  for (i=0; i<strlen(str); i++)
    if (strMember(str[i], charSet))
      count++;
  return count;
}

void strSubstr(char *substr, char *str, int i, int len) {
  strncpy(substr, &str[i], len);
  substr[len]='\0';
}

BOOL strPartOf(char *token, char *set) {
  ASSERT(token != NULL && set != NULL);
  ASSERT(strlen(token) < 100);
  char ttoken[100];
  sprintf(ttoken, "|%s|", token);
  return (strstr(set, ttoken)!=NULL);
}

void strTrim(char *trimStr, char *str, char *set) {
  char *start, *stop;
  for (start = str; strMember(*start, set); start++);
  for (stop = str+strlen(str)-1; stop > str && strMember(*stop, set); stop--);
  if (start <= stop) {
    strncpy(trimStr, start, stop-start+1);
    trimStr[stop-start+1]='\0';
  } else
    strcpy(trimStr, "");
}

void strReplace(char *str, char *from, char to) {
  int i;
  for (i=0; i<strlen(str); i++)
    if (strMember(str[i], from))
      str[i] = to;
}

char tspaces[MAX_LEN];
char* strSpaces(int level) {
  assert(level < MAX_LEN);
  memset(tspaces, ' ', MAX_LEN);
  tspaces[level] = '\0';
  return tspaces;
}

void strToUpper(char *str) {
  int i;
  for (i = 0; i<strlen(str); i++)
    str[i] = toupper(str[i]);
}

char *repl_str(const char *str, const char *old, const char *newstr) {

	/* Adjust each of the below values to suit your needs. */

	/* Increment positions cache size initially by this number. */
	size_t cache_sz_inc = 16;
	/* Thereafter, each time capacity needs to be increased,
	 * multiply the increment by this factor. */
	const size_t cache_sz_inc_factor = 2;
	/* But never increment capacity by more than this number. */
	const size_t cache_sz_inc_max = 1048576;

	char *pret, *ret = NULL;
	const char *pstr2, *pstr = str;
	size_t i, count = 0;
	ptrdiff_t *pos_cache = NULL;
	size_t cache_sz = 0;
	size_t cpylen, orglen, retlen, newlen, oldlen = strlen(old);

	/* Find all matches and cache their positions. */
	while ((pstr2 = strstr(pstr, old)) != NULL) {
		count++;

		/* Increase the cache size when necessary. */
		if (cache_sz < count) {
			cache_sz += cache_sz_inc;
			pos_cache = (ptrdiff_t *)realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
			if (pos_cache == NULL) {
				goto end_repl_str;
			}
			cache_sz_inc *= cache_sz_inc_factor;
			if (cache_sz_inc > cache_sz_inc_max) {
				cache_sz_inc = cache_sz_inc_max;
			}
		}

		pos_cache[count-1] = pstr2 - str;
		pstr = pstr2 + oldlen;
	}

	orglen = pstr - str + strlen(pstr);

	/* Allocate memory for the post-replacement string. */
	if (count > 0) {
		newlen = strlen(newstr);
		retlen = orglen + (newlen - oldlen) * count;
	} else	retlen = orglen;
	ret = (char *)malloc(retlen + 1);
	if (ret == NULL) {
		goto end_repl_str;
	}

	if (count == 0) {
		/* If no matches, then just duplicate the string. */
		strcpy(ret, str);
	} else {
		/* Otherwise, duplicate the string whilst performing
		 * the replacements using the position cache. */
		pret = ret;
		memcpy(pret, str, pos_cache[0]);
		pret += pos_cache[0];
		for (i = 0; i < count; i++) {
			memcpy(pret, newstr, newlen);
			pret += newlen;
			pstr = str + pos_cache[i] + oldlen;
			cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - oldlen;
			memcpy(pret, pstr, cpylen);
			pret += cpylen;
		}
		ret[retlen] = '\0';
	}

end_repl_str:
	/* Free the cache and return the post-replacement string,
	 * which will be NULL in the event of an error. */
	free(pos_cache);
	return ret;
}