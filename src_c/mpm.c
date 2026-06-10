#include "mpm.h"


MPM *MPM_new(
    float hertz,
    int substeps,
    int grid_width,
    int grid_height,
    size_t max_particles
) {
    MPM *mpm = malloc(sizeof(MPM));
    if (!mpm) {
        return NULL;
    }

    mpm->substeps = substeps;
    mpm->dt = 1.0f / hertz / (float)substeps;

    mpm->grid_width = grid_width;
    mpm->grid_height = grid_height;

    size_t n_cells = grid_width * grid_height;
    mpm->cells.velocity = malloc(sizeof(nvVector2) * n_cells);
    if (!(mpm->cells.velocity)) return NULL;
    mpm->cells.mass = malloc(sizeof(float) * n_cells);
    if (!(mpm->cells.mass)) return NULL;

    mpm->particles.C = malloc(sizeof(nvMatrix2) * max_particles);
    if (!(mpm->particles.C)) return NULL;
    mpm->particles.F = malloc(sizeof(nvMatrix2) * max_particles);
    if (!(mpm->particles.F)) return NULL;
    mpm->particles.position = malloc(sizeof(nvVector2) * max_particles);
    if (!(mpm->particles.position)) return NULL;
    mpm->particles.velocity = malloc(sizeof(nvVector2) * max_particles);
    if (!(mpm->particles.velocity)) return NULL;
    mpm->particles.mass = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.mass)) return NULL;
    mpm->particles.volume0 = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.volume0)) return NULL;

    return mpm;
}

void MPM_free(MPM *mpm) {
    if (!mpm) return;

    free(mpm->cells.velocity);
    free(mpm->cells.mass);

    free(mpm->particles.C);
    free(mpm->particles.F);
    free(mpm->particles.position);
    free(mpm->particles.velocity);
    free(mpm->particles.mass);
    free(mpm->particles.volume0);

    free(mpm);
}