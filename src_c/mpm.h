#ifndef NOVAPHYSICS_MPM_H
#define NOVAPHYSICS_MPM_H

#include <stdlib.h>
#include <stdint.h>
#include "vector.h"
#include "matrix.h"


typedef struct {
    nvMatrix2 *C;
    nvMatrix2 *F;
    nvVector2 *position;
    nvVector2 *velocity;
    float *mass;
    uint32_t *material;
    float *volume0;
    float *elastic_lambda;
    float *elastic_mu;
    float *rest_density;
    float *viscosity;
    float *tait_stiffness;
    float *tait_power;
} Particles;

typedef struct {
    nvVector2 *velocity;
    float *mass;
} Cells;

typedef struct {
    Particles particles;
    Cells cells;
    size_t grid_width;
    size_t grid_height;
    nvVector2 gravity;
    float dt;
    size_t substeps;

    size_t n_particles;
    size_t max_particles;
} MPM;

MPM *MPM_new(
    float hertz,
    size_t substeps,
    size_t grid_width,
    size_t grid_height,
    size_t max_particles
);

void MPM_free(MPM *mpm);

void MPM_add_elastic_particle(
    MPM *mpm,
    nvVector2 position,
    nvVector2 velocity,
    float mass,
    float elastic_lambda,
    float elastic_mu
);

void MPM_add_fluid_particle(
    MPM *mpm,
    nvVector2 position,
    nvVector2 velocity,
    float mass,
    float rest_density,
    float viscosity,
    float tait_stiffness,
    float tait_power
);

void MPM_precalc_volume(MPM *mpm);

void MPM_set_solver_settings(MPM *mpm, float hertz, int substeps);

void MPM_step(MPM *mpm);

void MPM_apply_brush(MPM *mpm, nvVector2 position, float radius, nvVector2 rel);


#endif // NOVAPHYSICS_MPM_H