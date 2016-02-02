// privileges.h -- Manage process privileges

#ifndef PRIVILEGES_H__
#define PRIVILEGES_H__

#include <sys/types.h>

extern int   runAsUser;
extern int   runAsGroup;

void lowerPrivileges(void);
void dropPrivileges(void);
const char *getUserName(uid_t uid);
uid_t getUserId(const char *name);
uid_t parseUserArg(const char *arg, const char **name);
const char *getGroupName(gid_t gid);
gid_t getGroupId(const char *name);
gid_t parseGroupArg(const char *arg, const char **name);

#ifndef HAVE_GETRESUID
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
#endif
#ifndef HAVE_GETRESGID
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
#endif
#ifndef HAVE_SETRESUID
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
#endif
#ifndef HAVE_SETRESGID
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
#endif

#endif
