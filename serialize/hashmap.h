#ifndef __HASH_T__
#define __HASH_T__

typedef struct ReturnHashmap { // used only for type 1
	char *key; // pointer to the char * used inside the actual hashmap for the key
			   // specific use case for this project only
	void **payload;
	int payload__length;
} hashmap__response;

void destroy__hashmap_response(hashmap__response *map_res);

typedef struct Store hashmap;

hashmap *make__hashmap(int hash__type, void (*printer)(void *), void (*destroy)(void *));

void *getKey__hashmap(hashmap *hash__m, void *key);
void **keys__hashmap(hashmap *hash__m, int *max_key, char *p, ...);
void *get__hashmap(hashmap *hash__m, void *key, int flag);

int print__hashmap(hashmap *hash__m);

int delete__hashmap(hashmap *hash__m, void *key);

int deepdestroy__hashmap(hashmap *hash);

int insert__hashmap(hashmap *hash__m, void *key, void *value, ...);

// simple key type functions
void printCharKey(void *characters);
int compareCharKey(void *characters, void *otherValue);
void destroyCharKey(void *characters);

void printIntKey(void *integer);
int compareIntKey(void *integer, void *otherValue);
void destroyIntKey(void *integer);

#endif