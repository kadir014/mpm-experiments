#ifndef NOVAPHYSICS_MPM_H
#define NOVAPHYSICS_MPM_H

#include <stdlib.h>
#include "vector.h"
#include "matrix.h"



typedef struct {
    nvMatrix2 *C;
    nvMatrix2 *F;
    nvVector2 *position;
    nvVector2 *velocity;
    float *mass;
    float *volume0;
} Particles;

typedef struct {
    nvVector2 *velocity;
    float *mass;
} Cells;

typedef struct {
    Particles particles;
    Cells cells;
    int grid_width;
    int grid_height;
    nvVector2 gravity;
    float dt;
    int substeps;
} MPM;

MPM *MPM_new(
    float hertz,
    int substeps,
    int grid_width,
    int grid_height,
    size_t max_particles
);

void MPM_free(MPM *mpm);


#endif // NOVAPHYSICS_MPM_H