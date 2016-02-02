#ifndef _READINI_H_
#define _READINI_H_

#ifdef __cplusplus
extern "C" {
#endif

int GetCurrentPath(char buf[],char *pFileName);
char *GetIniKeyString(char *title,char *key,char *filename);
int GetIniKeyInt(char *title,char *key,char *filename);

#ifdef __cplusplus
}
#endif

#endif