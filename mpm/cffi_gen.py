import os
import platform

from cffi import FFI


RELEASE_BUILD = True

MSVC_COMPILER_ARGS = (
    "/arch:AVX2",
    "/D_CRT_SECURE_NO_WARNINGS",
)

GCC_COMPILER_ARGS = (
    # Detect if building with python-wasm-sdk, emcc doesn't accept -march
    #"-march=native",
    "-D_POSIX_C_SOURCE=200809L",
)


def generate_cdef() -> None:
    """ Generate cdef header. """
    
    cdef_source = """
    typedef struct {
        float x;
        float y;
    } nvVector2;

    typedef struct {
        float m[4];
    } nvMatrix2;
    """

    bracket = 0
    staticinline = False
    with open(f"src_c/mpm.h", "r") as f:
        for line in f.readlines()[:-1]:

            if line.startswith("static inline"):
                if "{" in line: bracket += 1
                staticinline = True
                continue

            if staticinline and "{" in line:
                bracket += 1

            if staticinline and "}" in line:
                bracket -= 1

                if bracket == 0:
                    staticinline = False
                    continue

            if staticinline:
                continue

            if not (
                line.startswith("#ifndef NOVAPHYSICS") or
                line.startswith("#define NOVAPHYSICS") or
                line.startswith("#include") or
                line.startswith("#define")
            ):
                cdef_source += line

    with open("cdef.h", "w") as f:
        f.write(cdef_source)


generate_cdef()

ffibuilder = FFI()

with open("cdef.h", "r") as f:
    ffibuilder.cdef(f.read())

src_path = "src_c"
sources = []
for root, _, files in os.walk(src_path):
    for file in files:
        if file.endswith(".c"):
            sources.append(os.path.join(root, file))

c_args = []

if platform.system() == "Windows":
    c_args += MSVC_COMPILER_ARGS
else:
    c_args += GCC_COMPILER_ARGS

ffibuilder.set_source(
    "_mpm",
    "#include \"mpm.h\"",
    sources=sources,
    include_dirs=["src_c/"],
    extra_compile_args=c_args
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=False, debug=not RELEASE_BUILD)