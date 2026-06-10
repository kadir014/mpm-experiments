import _mpm

lib = _mpm.lib
ffi = _mpm.ffi


def adder(a: int, b: int) -> int:
    return lib.adder(a, b)