#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
int init(void);
int _store(void *data, unsigned int s1, unsigned int s2, int uuid, int prio);
int _get_size(int uuid, unsigned int *s1, unsigned int *s2);
int _fetch_to(void *data, int uuid);

#endif
