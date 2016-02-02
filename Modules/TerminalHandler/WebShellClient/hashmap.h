// hashmap.h -- Basic implementation of a hashmap abstract data type

#ifndef HASH_MAP__
#define HASH_MAP__

struct HashMap {
  void (*destructor)(void *arg, char *key, char *value);
  void *arg;
  struct {
    const char *key;
    const char *value;
  } **entries;
  int  mapSize;
  int  numEntries;
};

struct HashMap *newHashMap(void (*destructor)(void *arg, char *key, char *value),void *arg);
void initHashMap(struct HashMap *hashmap,
                 void (*destructor)(void *arg, char *key, char *value),
                 void *arg);
void destroyHashMap(struct HashMap *hashmap);
void deleteHashMap(struct HashMap *hashmap);
const void *addToHashMap(struct HashMap *hashmap, const char *key,
                         const char *value);
void deleteFromHashMap(struct HashMap *hashmap, const char *key);
char **getRefFromHashMap(const struct HashMap *hashmap, const char *key);
const char *getFromHashMap(const struct HashMap *hashmap, const char *key);
void iterateOverHashMap(struct HashMap *hashmap,
                        int (*fnc)(void *arg, const char *key, char **value),
                        void *arg);
int getHashmapSize(const struct HashMap *hashmap);

#endif /* HASH_MAP__ */
