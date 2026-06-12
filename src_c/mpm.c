#include "mpm.h"
#include <stdio.h>


static inline float clamp(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

static void MPM_clear_grid(MPM *mpm);
static void MPM_p2g0(MPM *mpm);
static void MPM_p2g(MPM *mpm);
static void MPM_grid_update(MPM *mpm);
static void MPM_g2p(MPM *mpm);

static inline void quadratic_kernel(
    nvVector2 cell_diff,
    nvVector2 weights[3]
) {
    weights[0] = nvVector2_mul(nvVector2_pow(nvVector2_sub(NV_VECTOR2(0.5f, 0.5f), cell_diff), 2.0f), 0.5f);
    weights[1] = nvVector2_sub(NV_VECTOR2(0.75f, 0.75f), nvVector2_pow(cell_diff, 2.0f));
    weights[2] = nvVector2_mul(nvVector2_pow(nvVector2_add(NV_VECTOR2(0.5f, 0.5f), cell_diff), 2.0f), 0.5f);
}


MPM *MPM_new(
    float hertz,
    size_t substeps,
    size_t grid_width,
    size_t grid_height,
    size_t max_particles
) {
    MPM *mpm = malloc(sizeof(MPM));
    if (!mpm) {
        return NULL;
    }

    MPM_set_solver_settings(mpm, hertz, substeps);

    mpm->grid_width = grid_width;
    mpm->grid_height = grid_height;

    mpm->gravity = NV_VECTOR2(0.0f, 9.81f);

    size_t n_cells = grid_width * grid_height;
    mpm->cells.velocity = malloc(sizeof(nvVector2) * n_cells);
    if (!(mpm->cells.velocity)) return NULL;
    mpm->cells.mass = malloc(sizeof(float) * n_cells);
    if (!(mpm->cells.mass)) return NULL;

    MPM_clear_grid(mpm);

    mpm->max_particles = max_particles;
    mpm->n_particles = 0;

    mpm->particles.material = malloc(sizeof(uint32_t) * max_particles);
    if (!(mpm->particles.material)) return NULL;
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
    mpm->particles.gravity_scale = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.gravity_scale)) return NULL;
    mpm->particles.volume0 = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.volume0)) return NULL;
    mpm->particles.elastic_lambda = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.elastic_lambda)) return NULL;
    mpm->particles.elastic_mu = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.elastic_mu)) return NULL;
    mpm->particles.rest_density = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.rest_density)) return NULL;
    mpm->particles.viscosity = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.viscosity)) return NULL;
    mpm->particles.tait_stiffness = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.tait_stiffness)) return NULL;
    mpm->particles.tait_power = malloc(sizeof(float) * max_particles);
    if (!(mpm->particles.tait_power)) return NULL;

    nvProfiler_reset(&mpm->profiler);

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
    free(mpm->particles.gravity_scale);
    free(mpm->particles.volume0);
    free(mpm->particles.elastic_lambda);
    free(mpm->particles.elastic_mu);

    free(mpm);
}

void MPM_add_elastic_particle(
    MPM *mpm,
    nvVector2 position,
    nvVector2 velocity,
    float mass,
    float gravity_scale,
    float elastic_lambda,
    float elastic_mu
) {
    if (mpm->n_particles >= mpm->max_particles) return;

    mpm->particles.material[mpm->n_particles] = 0;
    mpm->particles.position[mpm->n_particles] = position;
    mpm->particles.velocity[mpm->n_particles] = velocity;
    mpm->particles.mass[mpm->n_particles] = mass;
    mpm->particles.gravity_scale[mpm->n_particles] = gravity_scale;
    mpm->particles.C[mpm->n_particles] = nvMatrix2_zero;
    mpm->particles.F[mpm->n_particles] = nvMatrix2_identity;
    mpm->particles.elastic_lambda[mpm->n_particles] = elastic_lambda;
    mpm->particles.elastic_mu[mpm->n_particles] = elastic_mu;
    mpm->particles.volume0[mpm->n_particles] = 0.0;

    mpm->particles.rest_density[mpm->n_particles] = 0.0;
    mpm->particles.viscosity[mpm->n_particles] = 0.0;
    mpm->particles.tait_stiffness[mpm->n_particles] = 0.0;
    mpm->particles.tait_power[mpm->n_particles] = 0.0;

    mpm->n_particles++;
}

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
) {
    if (mpm->n_particles >= mpm->max_particles) return;

    mpm->particles.material[mpm->n_particles] = 1;
    mpm->particles.position[mpm->n_particles] = position;
    mpm->particles.velocity[mpm->n_particles] = velocity;
    mpm->particles.mass[mpm->n_particles] = mass;
    mpm->particles.gravity_scale[mpm->n_particles] = gravity_scale;
    mpm->particles.C[mpm->n_particles] = nvMatrix2_zero;
    mpm->particles.F[mpm->n_particles] = nvMatrix2_identity;
    mpm->particles.rest_density[mpm->n_particles] = rest_density;
    mpm->particles.viscosity[mpm->n_particles] = viscosity;
    mpm->particles.tait_stiffness[mpm->n_particles] = tait_stiffness;
    mpm->particles.tait_power[mpm->n_particles] = tait_power;

    mpm->particles.elastic_lambda[mpm->n_particles] = 0.0;
    mpm->particles.elastic_mu[mpm->n_particles] = 0.0;
    mpm->particles.volume0[mpm->n_particles] = 0.0;

    mpm->n_particles++;
}

void MPM_clear(MPM *mpm) {
    MPM_clear_grid(mpm);
    mpm->n_particles = 0;
}

void MPM_set_solver_settings(MPM *mpm, float hertz, int substeps) {
    mpm->substeps = substeps;
    mpm->dt = 1.0f / hertz / (float)substeps;
}

void MPM_precalc_volume(MPM *mpm) {
    MPM_p2g0(mpm);

    for (size_t i = 0; i < mpm->n_particles; i++) {
        uint32_t cell_idx_x = (uint32_t)mpm->particles.position[i].x;
        uint32_t cell_idx_y = (uint32_t)mpm->particles.position[i].y;
        nvVector2 cell_diff = nvVector2_sub(
            nvVector2_sub(mpm->particles.position[i], NV_VECTOR2(cell_idx_x, cell_idx_y)),
            NV_VECTOR2(0.5f, 0.5f)
        );

        nvVector2 weights[3];
        quadratic_kernel(cell_diff, weights);

        float density = 0.0;

        // 3x3 neighboring cells
        for (size_t gx = 0; gx < 3; gx++) {
            for (size_t gy = 0; gy < 3; gy++) {
                float weight = weights[gx].x * weights[gy].y;

                nvVector2 nb_cell_pos = NV_VECTOR2(
                    (float)(cell_idx_x + gx - 1),
                    (float)(cell_idx_y + gy - 1)
                );
                uint32_t nb_cell_idx = (uint32_t)nb_cell_pos.y * mpm->grid_width + (uint32_t)nb_cell_pos.x;

                density += mpm->cells.mass[nb_cell_idx] * weight;
            }
        }

        // Since this is 2d, "area" might be more correct
        float volume = mpm->particles.mass[i] / density;
        mpm->particles.volume0[i] = volume;
    }
}

#define TIMER_START nvPrecisionTimer_start(&timer)
#define TIMER_STOP(field) mpm->profiler.field += nvPrecisionTimer_stop(&timer);

void MPM_step(MPM *mpm) {
    nvProfiler_reset(&mpm->profiler);

    nvPrecisionTimer step_timer;
    nvPrecisionTimer_start(&step_timer);

    for (size_t substep = 0; substep < mpm->substeps; substep++) {
        nvPrecisionTimer timer;

        TIMER_START;
        MPM_clear_grid(mpm);
        TIMER_STOP(clear_grid);

        // P2G 0 -> contribute mass
        // P2G 1 -> contribute material

        TIMER_START;
        MPM_p2g0(mpm);
        TIMER_STOP(p2g0);

        TIMER_START;
        MPM_p2g(mpm);
        TIMER_STOP(p2g1);

        TIMER_START;
        MPM_grid_update(mpm);
        TIMER_STOP(update_grid);

        TIMER_START;
        MPM_g2p(mpm);
        TIMER_STOP(g2p);
    }

    mpm->profiler.step += nvPrecisionTimer_stop(&step_timer);
}

static void MPM_clear_grid(MPM *mpm) {
    for (size_t i = 0; i < mpm->grid_width * mpm->grid_height; i++) {
        mpm->cells.velocity[i] = nvVector2_zero;
        mpm->cells.mass[i] = 0.0;
    }
}

static void MPM_p2g0(MPM *mpm) {
    for (size_t i = 0; i < mpm->n_particles; i++) {
        uint32_t cell_idx_x = (uint32_t)mpm->particles.position[i].x;
        uint32_t cell_idx_y = (uint32_t)mpm->particles.position[i].y;
        nvVector2 cell_diff = nvVector2_sub(
            nvVector2_sub(mpm->particles.position[i], NV_VECTOR2(cell_idx_x, cell_idx_y)),
            NV_VECTOR2(0.5f, 0.5f)
        );

        nvVector2 weights[3];
        quadratic_kernel(cell_diff, weights);

        nvMatrix2 C = mpm->particles.C[i];

        // 3x3 neighboring cells
        for (size_t gx = 0; gx < 3; gx++) {
            for (size_t gy = 0; gy < 3; gy++) {
                float weight = weights[gx].x * weights[gy].y;

                nvVector2 nb_cell_pos = NV_VECTOR2(
                    (float)(cell_idx_x + gx - 1),
                    (float)(cell_idx_y + gy - 1)
                );
                nvVector2 nb_cell_dist = nvVector2_add(
                    nvVector2_sub(nb_cell_pos, mpm->particles.position[i]),
                    NV_VECTOR2(0.5f, 0.5f)
                );
                uint32_t nb_cell_idx = (uint32_t)nb_cell_pos.y * mpm->grid_width + (uint32_t)nb_cell_pos.x;

                nvVector2 Q = nvMatrix2_mulv(C, nb_cell_dist);

                float weighted_mass = mpm->particles.mass[i] * weight;
                mpm->cells.mass[nb_cell_idx] += weighted_mass;

                mpm->cells.velocity[nb_cell_idx] = nvVector2_add(
                    mpm->cells.velocity[nb_cell_idx],
                    nvVector2_mul(nvVector2_add(mpm->particles.velocity[i], Q), weighted_mass)
                );
            }
        }
    }
}

static void MPM_p2g(MPM *mpm) {
    for (size_t i = 0; i < mpm->n_particles; i++) {
        uint32_t cell_idx_x = (uint32_t)mpm->particles.position[i].x;
        uint32_t cell_idx_y = (uint32_t)mpm->particles.position[i].y;
        nvVector2 cell_diff = nvVector2_sub(
            nvVector2_sub(mpm->particles.position[i], NV_VECTOR2(cell_idx_x, cell_idx_y)),
            NV_VECTOR2(0.5f, 0.5f)
        );

        nvVector2 weights[3];
        quadratic_kernel(cell_diff, weights);

        // find a better name for this
        nvMatrix2 eq_16_term_0 = nvMatrix2_zero;

        // Elastic        
        if (mpm->particles.material[i] == 0) {
            nvMatrix2 F = mpm->particles.F[i];

            float J = nvMatrix2_determinant(F);
            float volume = mpm->particles.volume0[i] * J;

            // Useful matrices for Neo-Hookean model
            nvMatrix2 F_T = nvMatrix2_transpose(F);
            nvMatrix2 F_inv_T = nvMatrix2_inverse(F_T);
            nvMatrix2 F_minus_F_inv_T = nvMatrix2_sub(F, F_inv_T);

            float J_log = logf(J);
            nvMatrix2 P_term_0 = nvMatrix2_muls(F_minus_F_inv_T, mpm->particles.elastic_mu[i]);
            nvMatrix2 P_term_1 = nvMatrix2_muls(F_inv_T, mpm->particles.elastic_lambda[i] * J_log);
            nvMatrix2 P = nvMatrix2_add(P_term_0, P_term_1);

            // Cauchy stress
            nvMatrix2 stress = nvMatrix2_zero;
            if (fabsf(J) > 0.0001f) {
                stress = nvMatrix2_muls(nvMatrix2_mulm(P, F_T), 1.0f / J);
            }

            eq_16_term_0 = nvMatrix2_muls(stress, -volume * 4.0f);
            eq_16_term_0 = nvMatrix2_muls(eq_16_term_0, mpm->dt);
        }
        // Fluid
        else if (mpm->particles.material[i] == 1) {
            // estimating particle volume by summing up neighbourhood's weighted mass contribution
            // MPM course, equation 152
            float density = 0.0f;

            for (size_t gx = 0; gx < 3; gx++) {
                for (size_t gy = 0; gy < 3; gy++) {
                    float weight = weights[gx].x * weights[gy].y;

                    nvVector2 nb_cell_pos = NV_VECTOR2(
                        (float)(cell_idx_x + gx - 1),
                        (float)(cell_idx_y + gy - 1)
                    );
                    uint32_t nb_cell_idx = (uint32_t)nb_cell_pos.y * mpm->grid_width + (uint32_t)nb_cell_pos.x;

                    density += mpm->cells.mass[nb_cell_idx] * weight;
                }
            }

            float volume = mpm->particles.mass[i] / density;

            /*
                Constitutive equation for isotropic fluid: 
                stress = -pressure * I + viscosity * (velocity_gradient + velocity_gradient_transposed)

                Tait equation of state for pressure
            */
            float pressure = powf(density / mpm->particles.rest_density[i], mpm->particles.tait_power[i]) - 1.0f;
            pressure *= mpm->particles.tait_stiffness[i];

            // Negative pressure is clamped in Nial's article
            if (pressure < -0.1f) pressure = -0.1f;

            nvMatrix2 stress = NV_MATRIX2(
                -pressure, 0,
                0,         -pressure
            );

            // velocity gradient - CPIC eq. 17, where deriv of quadratic polynomial is linear
            nvMatrix2 dudv = mpm->particles.C[i];
            nvMatrix2 strain = dudv;

            float trace = strain.m[2] + strain.m[1];
            strain.m[1] = trace;
            strain.m[2] = trace;

            nvMatrix2 viscosity_term = nvMatrix2_muls(strain, mpm->particles.viscosity[i]);
            stress = nvMatrix2_add(stress, viscosity_term);

            eq_16_term_0 = nvMatrix2_muls(stress, -volume * 4.0f);
            eq_16_term_0 = nvMatrix2_muls(eq_16_term_0, mpm->dt);
        }

        // 3x3 neighboring cells
        for (size_t gx = 0; gx < 3; gx++) {
            for (size_t gy = 0; gy < 3; gy++) {
                float weight = weights[gx].x * weights[gy].y;

                nvVector2 nb_cell_pos = NV_VECTOR2(
                    (float)(cell_idx_x + gx - 1),
                    (float)(cell_idx_y + gy - 1)
                );
                nvVector2 nb_cell_dist = nvVector2_add(
                    nvVector2_sub(nb_cell_pos, mpm->particles.position[i]),
                    NV_VECTOR2(0.5f, 0.5f)
                );
                uint32_t nb_cell_idx = (uint32_t)nb_cell_pos.y * mpm->grid_width + (uint32_t)nb_cell_pos.x;

                // "Fake" smoke buoyancy
                mpm->cells.velocity[nb_cell_idx] = nvVector2_add(
                    mpm->cells.velocity[nb_cell_idx],
                    nvVector2_mul(mpm->gravity, (mpm->particles.gravity_scale[i] * weight * mpm->dt))
                );

                // Fused force + momentum contribution from MLS-MPM
                nvVector2 momentum = nvMatrix2_mulv(
                    nvMatrix2_muls(eq_16_term_0, weight), nb_cell_dist
                );
                mpm->cells.velocity[nb_cell_idx] = nvVector2_add(
                    mpm->cells.velocity[nb_cell_idx], momentum
                );

                /*
                    IMPORTANT: cell velocity currently refers to momentum, not velocity.
                               It is reverted back to velocity in grid update step.
                */
            }
        }
    }
}

static void MPM_grid_update(MPM *mpm) {
    for (size_t i = 0; i < mpm->grid_width * mpm->grid_height; i++) {
        if (mpm->cells.mass[i] <= 0.0f) {
            continue;
        }

        // Convert momentum back to velocity (and apply external forces)
        mpm->cells.velocity[i] = nvVector2_div(mpm->cells.velocity[i], mpm->cells.mass[i]);
        nvVector2 external = mpm->gravity;
        mpm->cells.velocity[i] = nvVector2_add(
            mpm->cells.velocity[i],
            nvVector2_mul(external, mpm->dt)
        );

        // Boundary conditions
        size_t x = i % mpm->grid_width;
        size_t y = i / mpm->grid_width;
        if (x < 2 || x > mpm->grid_width - 3) {
            mpm->cells.velocity[i].x = 0.0f;
            //mpm->cells.velocity[i].y *= 0.3f;
        }
        if (y < 2 || y > mpm->grid_height - 3) {
            mpm->cells.velocity[i].y = 0.0f;
            //mpm->cells.velocity[i].x *= 0.3f;
        }

        // Degenerate velocities
        float degen = 350.0f;
        if (nvVector2_dot(mpm->cells.velocity[i], mpm->cells.velocity[i]) > degen * degen) {
            mpm->cells.velocity[i] = nvVector2_zero;
        }
    }
}

static void MPM_g2p(MPM *mpm) {
    for (size_t i = 0; i < mpm->n_particles; i++) {
        // Reset particle velocity because we calculate it from scratch each step
        // using the updated grid velocities.
        mpm->particles.velocity[i] = nvVector2_zero;

        uint32_t cell_idx_x = (uint32_t)mpm->particles.position[i].x;
        uint32_t cell_idx_y = (uint32_t)mpm->particles.position[i].y;
        nvVector2 cell_diff = nvVector2_sub(
            nvVector2_sub(mpm->particles.position[i], NV_VECTOR2(cell_idx_x, cell_idx_y)),
            NV_VECTOR2(0.5f, 0.5f)
        );

        nvVector2 weights[3];
        quadratic_kernel(cell_diff, weights);

        // Constructing affine per-particle momentum matrix from APIC / MLS-MPM
        nvMatrix2 B = nvMatrix2_zero;

        // 3x3 neighboring cells
        for (size_t gx = 0; gx < 3; gx++) {
            for (size_t gy = 0; gy < 3; gy++) {
                float weight = weights[gx].x * weights[gy].y;

                nvVector2 nb_cell_pos = NV_VECTOR2(
                    (float)(cell_idx_x + gx - 1),
                    (float)(cell_idx_y + gy - 1)
                );
                nvVector2 nb_cell_dist = nvVector2_add(
                    nvVector2_sub(nb_cell_pos, mpm->particles.position[i]),
                    NV_VECTOR2(0.5f, 0.5f)
                );
                uint32_t nb_cell_idx = (uint32_t)nb_cell_pos.y * mpm->grid_width + (uint32_t)nb_cell_pos.x;

                nvVector2 weighted_vel = nvVector2_mul(mpm->cells.velocity[nb_cell_idx], weight);

                mpm->particles.velocity[i] = nvVector2_add(mpm->particles.velocity[i], weighted_vel);

                // APIC paper eq10, constructing inner term for B
                nvMatrix2 term = NV_MATRIX2(
                    weighted_vel.x * nb_cell_dist.x,
                    weighted_vel.y * nb_cell_dist.x,
                    weighted_vel.x * nb_cell_dist.y,
                    weighted_vel.y * nb_cell_dist.y
                );

                B = nvMatrix2_add(B, term);
            }
        }

        // TODO: Magic number 4?
        mpm->particles.C[i] = nvMatrix2_muls(B, 4.0f);

        // Advect particles
        mpm->particles.position[i] = nvVector2_add(
            mpm->particles.position[i],
            nvVector2_mul(mpm->particles.velocity[i], mpm->dt)
        );

        // Safety clamp to ensure particles don't exit domain
        mpm->particles.position[i].x = clamp(mpm->particles.position[i].x, 1.0, mpm->grid_width-2);
        mpm->particles.position[i].y = clamp(mpm->particles.position[i].y, 1.0, mpm->grid_height-2);
    
        // Deformation gradient update - MPM course, eq.181
        nvMatrix2 Fp_new = nvMatrix2_identity;
        Fp_new = nvMatrix2_add(Fp_new, nvMatrix2_muls(mpm->particles.C[i], mpm->dt));
        mpm->particles.F[i] = nvMatrix2_mulm(Fp_new, mpm->particles.F[i]);

        // TODO: Hacky clamp
        if (nvMatrix2_determinant(mpm->particles.F[i]) < 0.1f) {
            mpm->particles.F[i] = nvMatrix2_identity;
        }

        // Soft boundaries
        // TODO: Predicted position needs vel * dt?
        nvVector2 next_pos = nvVector2_add(mpm->particles.position[i], nvVector2_mul(mpm->particles.velocity[i], mpm->dt));
        float damping = 0.75f;
        float wmin = 3.0f;
        float xmax = (float)mpm->grid_width - (wmin + 1.0f);
        float ymax = (float)mpm->grid_height - (wmin + 1.0f);
        if (next_pos.x < wmin) mpm->particles.velocity[i].x += (wmin - next_pos.x) * damping;
        if (next_pos.y < wmin) mpm->particles.velocity[i].y += (wmin - next_pos.y) * damping;
        if (next_pos.x > xmax) mpm->particles.velocity[i].x += (xmax - next_pos.x) * damping;
        if (next_pos.y > ymax) mpm->particles.velocity[i].y += (ymax - next_pos.y) * damping;
    }
}

void MPM_apply_brush(MPM *mpm, nvVector2 position, float radius, nvVector2 rel) {
    for (size_t i = 0; i < mpm->n_particles; i++) {
        nvVector2 delta = nvVector2_sub(mpm->particles.position[i], position);
        if (nvVector2_dot(delta, delta) < radius * radius) {
            // Mode: Drag
            nvVector2 drag = nvVector2_mul(rel, 1.0f / ((float)mpm->substeps * mpm->dt));
            mpm->particles.velocity[i] = drag;
            //mpm->particles.position[i] = nvVector2_add(mpm->particles.position[i], drag);
        }
    }
}

void MPM_get_particle_view(
    MPM *mpm,
    char *target,
    float zoom,
    size_t width,
    size_t height
) {
    for (size_t i = 0; i < mpm->n_particles; i++) {
        nvVector2 pos = mpm->particles.position[i];
        
        // world -> screen
        pos.x *= zoom;
        pos.y *= zoom;

        // screen -> ndc
        pos.x = pos.x / (float)width * 2.0f - 1.0f;
        pos.y = ((float)height - pos.y) / (float)height * 2.0f - 1.0f;

        // TODO: Which is faster? memcpy or casting to float array?
        // size_t offset = i * sizeof(float) * 2;
        // memcpy(target + offset, &pos.x, sizeof(float));
        // memcpy(target + offset + sizeof(float), &pos.y, sizeof(float));

        float *out = (float *)target;
        out[i * 2 + 0] = pos.x;
        out[i * 2 + 1] = pos.y;
    }
}