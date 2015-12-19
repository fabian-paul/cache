import cache
import numpy as np
from time import sleep

for i in range(1024):
   print i
   z = np.zeros((1024,1024))
   cache.cache.store(z, i, 0)
   sleep(0.1)
