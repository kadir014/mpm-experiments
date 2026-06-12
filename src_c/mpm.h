#ifndef NOVAPHYSICS_MPM_H
#define NOVAPHYSICS_MPM_H

#include <stdlib.h>
#include <stdint.h>
#include "vector.h"
#include "matrix.h"
#include "profiler.h"


typedef struct {
    nvMatrix2 *C;
    nvMatrix2 *F;
    nvVector2 *position;
    nvVector2 *velocity;
    float *mass;
    float *gravity_scale;
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

    nvProfiler profiler;
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
    float gravity_scale,
    float elastic_lambda,
    float elastic_mu
);

void MPM_add_fluid_particle(
    MPM *mpm,
    nvVector2 position,
    nvVector2 velocity,
    float mass,
    float gravity_scale,
    float rest_density,
    float viscosity,
    float tait_stiffness,
    float tait_power
);

void MPM_clear(MPM *mpm);

void MPM_precalc_volume(MPM *mpm);

void MPM_set_solver_settings(MPM *mpm, float hertz, int substeps);

void MPM_step(MPM *mpm);

void MPM_apply_brush(MPM *mpm, nvVector2 position, float radius, nvVector2 rel);

void MPM_get_particle_view(
    MPM *mpm,
    char *target,
    float zoom,
    size_t width,
    size_t height
);


#endif // NOVAPHYSICS_MPM_H