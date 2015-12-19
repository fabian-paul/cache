#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include "avl/avl.h"

static unsigned int get_free_mem()
{
    FILE *f;
    int n;
    char *start, *end;
    char buffer[0x1000];
    f = fopen("/proc/meminfo","r");
    if(!f) return 0;
    n = fread(buffer, 1, 0x1000, f);
    if(n<1) { fclose(f); return 0; }
    fclose(f);
    buffer[0x1000-1] = '\0';
    start = strstr(buffer, "MemFree:");
    if(start==NULL) return 0;
    end = strchr(start,'\n');
    if(end==NULL) return 0;
    *end = '\0';
    return (unsigned int)atoi(start+8) * 1024;
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int threshold = 1024*1024*1024; /* 1GB */

typedef struct struct_entry {
    int prio;
    int uuid;
    unsigned int s1, s2;
    void *data;
} entry;

static entry *new_entry(int prio, int uuid, unsigned int s1, unsigned int s2, void *data)
{
    entry *e;
    e = malloc(sizeof(entry));
    if(!e) return NULL;
    e->prio = prio;
    e->uuid = uuid;
    e->s1 = s1;
    e->s2 = s2;
    e->data = data;
    return e;
}

TREE *pages_by_uuid;
TREE *pages_by_prio;

static unsigned int delete_page(void)
{
    entry *page, *ok;
    unsigned int len;
    page = avl_locate_first(pages_by_prio);
    if(!page) return 0;
    ok = avl_remove_int(pages_by_uuid, page->uuid);
    if(!ok) return 0;
    ok = avl_remove_int(pages_by_prio, page->prio);
    if(!ok) return 0;
    free(page->data);
    len = page->s1 * page->s2 * sizeof(double);
    free(page);
    return len;
}

static void delete_pages(unsigned int n_bytes, int bg)
{
    unsigned int sum, deleted;
    printf("deleting %ud kb (bg=%d)\n", n_bytes/1024, bg);
    pthread_mutex_lock(&mutex);
    sum = 0;
    do {
        deleted = delete_page();
        if(deleted==0) break; /* failed */
        sum += deleted;
    } while(sum < n_bytes);
    malloc_trim(0);
    pthread_mutex_unlock(&mutex);
}

static void *cache(void *ptr)
{
    unsigned int mf;

    while(1) {
        sleep(1);
        mf = get_free_mem();
        if(mf < threshold)
            delete_pages(threshold-mf, 1);
    }
    return NULL;
}

int init(void) {
    int err;
    pthread_t thread;

    setlocale(LC_NUMERIC,"C");
    pages_by_uuid = avl_tree_nodup_int(entry, uuid);
    if(!pages_by_uuid) return -1;
    pages_by_prio = avl_tree_dup_int(entry, prio);
    if(!pages_by_prio) return -1;
    err = pthread_create(&thread, NULL, cache, NULL);
    if(err!=0) return err;
    return 0;
}

int _store(void *data, unsigned int s1, unsigned int s2, int uuid, int prio) {
    unsigned int mf, len;
    int ok;
    void *ptr;
    entry *page;

    len = s1*s2*sizeof(double);
    mf = get_free_mem();
    printf("free mem= %d kB\n", mf/1024);

    if(mf < threshold) {
        delete_pages(threshold-mf, 0);
        return -2;
    } else {
        if(mf < threshold+len) return -2;
        else {
            ptr = malloc(len);
            if(!ptr) return -1;
            memcpy(ptr, data, len);
            page = new_entry(prio, uuid, s1, s2, ptr);
            if(!page) { free(ptr); return -1; }
            pthread_mutex_lock(&mutex);
            ok = avl_insert(pages_by_uuid, page);
            if(!ok) { free(page); free(ptr); pthread_mutex_unlock(&mutex); return -1; }
            ok = avl_insert(pages_by_prio, page);
            if(!ok) { avl_remove_int(pages_by_uuid, page->uuid); free(page); free(ptr); pthread_mutex_unlock(&mutex); return -1; }
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
}

int _get_size(int uuid, unsigned int *s1, unsigned int *s2)
{
    entry *page;
    pthread_mutex_lock(&mutex);
    page = avl_locate_int(pages_by_uuid, uuid);
    if(!page) { pthread_mutex_unlock(&mutex); return -1; }
    *s1 = page->s1;
    *s2 = page->s2;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int _fetch_to(void *buffer, int uuid)
{
    entry *page;
    unsigned int len;
    pthread_mutex_lock(&mutex);
    page = avl_locate_int(pages_by_uuid, uuid);
    if(!page) { pthread_mutex_unlock(&mutex); return -1; }
    len = page->s1 * page->s2 * sizeof(double);
    memcpy(buffer, page->data, len);
    pthread_mutex_unlock(&mutex);
    return 0;
}
