import setuptools
from setuptools import setup
from distutils.core import Extension
from Cython.Build import cythonize
import numpy as np

ext_cache = Extension(
        "cache.cache",
        sources=["cache.pyx", "_cache.c", "avl/avl.c" ],
        include_dirs=["./avl", np.get_include()]
    )

if __name__ == "__main__":
    setup(name = 'cache',
          packages=['cache'],
          ext_modules = cythonize([ext_cache]),
    )

