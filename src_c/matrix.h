#ifndef NOVAPHYSICS_MATRIX_H
#define NOVAPHYSICS_MATRIX_H


/**
 * @brief 2x2 matrix in column-major order.
 */
typedef struct {
    float m[4];
} nvMatrix2;


/**
 * @brief Initialize nsMatrix2 literal.
 * 
 * @param m Array of elements in column-major order.
 * @return nvMatrix2
 */
#define NV_MATRIX2(m) ((nvMatrix4){{m}})


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


#endif // NOVAPHYSICS_MATRIX_H