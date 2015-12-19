import numpy as np

cdef extern from "_cache.h":
    int init()
    int _store(const void * const data, unsigned int s1, unsigned int s2, int uuid, int prio)
    int _get_size(int uuid, unsigned int *s1, unsigned int *s2)
    int _fetch_to(void *buffer, int uuid)

init()

def store(double[:,::1] data, uuid, prio):
    cdef double[:,::1] c_data = data
    res = _store(&c_data[0,0], data.shape[0], data.shape[1], uuid, prio)
    if res==-2:
        print 'cache is full'
    if res not in [0,-2]:
        raise Exception('failed to store')
    
def fetch(uuid):
    cdef unsigned int s1, s2
    res = _get_size(np.intc(uuid), &s1, &s2)
    if res!=0:
        return None
    cdef double[:,::1] data = np.empty((s1,s2), dtype=np.float64)
    res = _fetch_to(&data[0,0], np.intc(uuid))
    if res!=0:
        return None
    else:
        return np.asarray(data)
    