#ifndef NOVAPHYSICS_MATRIX_H
#define NOVAPHYSICS_MATRIX_H

#include "vector.h"


/**
 * @brief 2x2 matrix in column-major order.
 */
typedef struct {
    float m[4];
} nvMatrix2;


/**
 * @brief Initialize nvMatrix2 literal.
 * 
 * @param m Array of elements in column-major order.
 * @return nvMatrix2
 */
#define NV_MATRIX2(...) ((nvMatrix2){{__VA_ARGS__}})

/**
 * @brief Constant 2x2 zero matrix.
 */
static const nvMatrix2 nvMatrix2_zero = {
    {
        0.0f, 0.0f,
        0.0f, 0.0f
    }
};


/**
 * @brief Constant 2x2 identity matrix.
 */
static const nvMatrix2 nvMatrix2_identity = {
    {
        1.0f, 0.0f,
        0.0f, 1.0f
    }
};


#define nvMatrix2_get(mat, row, col) (mat.m[(col) * 2 + (row)])

#define nvMatrix2_set(mat, row, col, value) (mat.m[(col) * 2 + (row)] = (value))


static inline nvMatrix2 nvMatrix2_add(nvMatrix2 a, nvMatrix2 b) {
    return NV_MATRIX2(
        a.m[0] + b.m[0],
        a.m[1] + b.m[1],
        a.m[2] + b.m[2],
        a.m[3] + b.m[3]
    );
}

static inline nvMatrix2 nvMatrix2_sub(nvMatrix2 a, nvMatrix2 b) {
    return NV_MATRIX2(
        a.m[0] - b.m[0],
        a.m[1] - b.m[1],
        a.m[2] - b.m[2],
        a.m[3] - b.m[3]
    );
}

static inline nvMatrix2 nvMatrix2_muls(nvMatrix2 m, float s) {
    return NV_MATRIX2(
        m.m[0] * s,
        m.m[1] * s,
        m.m[2] * s,
        m.m[3] * s
    );
}

static inline nvVector2 nvMatrix2_mulv(nvMatrix2 m, nvVector2 v) {
    return NV_VECTOR2(
        m.m[0] * v.x + m.m[2] * v.y,
        m.m[1] * v.x + m.m[3] * v.y
    );
}

static inline nvMatrix2 nvMatrix2_mulm(nvMatrix2 a, nvMatrix2 b) {
    return NV_MATRIX2(
        a.m[0] * b.m[0] + a.m[2] * b.m[1],
        a.m[1] * b.m[0] + a.m[3] * b.m[1],
        a.m[0] * b.m[2] + a.m[2] * b.m[3],
        a.m[1] * b.m[2] + a.m[3] * b.m[3]
    );
}

static inline float nvMatrix2_determinant(nvMatrix2 m) {
    return m.m[0] * m.m[3] - m.m[2] * m.m[1];
}

static inline nvMatrix2 nvMatrix2_transpose(nvMatrix2 m) {
    return NV_MATRIX2(
        m.m[0],
        m.m[2],
        m.m[1],
        m.m[3]
    );
}

static inline nvMatrix2 nvMatrix2_inverse(nvMatrix2 m) {
    float det = nvMatrix2_determinant(m);

    if (fabsf(det) < 0.000001f) {
        return nvMatrix2_zero;
    }

    float inv_det = 1.0f / det;
    return NV_MATRIX2(
        m.m[3] * inv_det,
        -m.m[1] * inv_det,
        -m.m[2] * inv_det,
        m.m[0] * inv_det
    );
}


#endif // NOVAPHYSICS_MATRIX_H