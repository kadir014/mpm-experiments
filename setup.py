from setuptools import setup


setup(
    packages=["mpm"],
    cffi_modules=["mpm/cffi_gen.py:ffibuilder"],
)