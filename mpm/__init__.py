from collections.abc import Iterator
from dataclasses import dataclass

import _mpm
from mpm.vector import Vector2


lib = _mpm.lib
ffi = _mpm.ffi


@dataclass
class Profiler:
    step: float = 0.0
    clear_grid: float = 0.0
    p2g0: float = 0.0
    p2g1: float = 0.0
    update_grid: float = 0.0
    g2p: float = 0.0


class MPM:
    def __init__(self,
            grid_size: tuple[int, int],
            hertz: float = 60.0,
            substeps: int = 1
            ) -> None:
        self.__hertz = hertz
        self._mpm = lib.MPM_new(
            hertz, substeps, grid_size[0], grid_size[1], 100_000
        )

        if self._mpm == ffi.NULL:
            raise Exception("Low level error.")
        
        self.profiler = Profiler()
        
    @property
    def n_particles(self) -> int:
        return self._mpm.n_particles
    
    @property
    def max_particles(self) -> int:
        return self._mpm.max_particles
    
    @property
    def hertz(self) -> float:
        return self.__hertz
    
    @property
    def substeps(self) -> int:
        return self._mpm.substeps
    
    @property
    def gravity(self) -> Vector2:
        return Vector2(self._mpm.gravity.x, self._mpm.gravity.y)
    
    @gravity.setter
    def gravity(self, value: Vector2) -> None:
        self._mpm.gravity.x = value.x
        self._mpm.gravity.y = value.y
        
    def __del__(self) -> None:
        if not hasattr(self, "_mpm"): return

        lib.MPM_free(self._mpm)
        del self._mpm

    def cell(self, x: int, y: int) -> tuple[Vector2, float]:
        x = int(x)
        y = int(y)

        idx = y * 100 + x

        vx = self._mpm.cells.velocity[idx].x
        vy = self._mpm.cells.velocity[idx].y
        m = self._mpm.cells.mass[idx]

        return (Vector2(vx, vy), m)

    def set_solver_settings(self, hertz: float = 60.0, substeps: int = 1) -> None:
        self.__hertz = hertz
        lib.MPM_set_solver_settings(self._mpm, hertz, substeps)

    def iter_particles(self) -> Iterator[tuple[Vector2, float, int]]:
        n_particles = self._mpm.n_particles

        for i in range(n_particles):
            yield (
                Vector2(
                    self._mpm.particles.position[i].x,
                    self._mpm.particles.position[i].y
                ),
                self._mpm.particles.elastic_mu[i],
                self._mpm.particles.material[i]
            )

    def add_elastic_particle(self,
            position: Vector2,
            velocity: Vector2 = Vector2(0.0, 0.0),
            mass: float = 1.0,
            elastic_lambda: float = 5000.0,
            elastic_mu: float = 10000.0
            ) -> None:
        lib.MPM_add_elastic_particle(
            self._mpm,
            (position.x, position.y),
            (velocity.x, velocity.y),
            mass,
            0.0,
            elastic_lambda,
            elastic_mu
        )

    def add_fluid_particle(self,
            position: Vector2,
            velocity: Vector2 = Vector2(0.0, 0.0),
            mass: float = 1.0,
            gravity_scale: float = 0.0,
            rest_density: float = 2.0,
            viscosity: float = 0.0,
            tait_stiffness: float = 185.0,
            tait_power: float = 6.0
            ) -> None:
        lib.MPM_add_fluid_particle(
            self._mpm,
            (position.x, position.y),
            (velocity.x, velocity.y),
            mass,
            gravity_scale,
            rest_density,
            viscosity,
            tait_stiffness,
            tait_power
        )

    def clear(self) -> None:
        lib.MPM_clear(self._mpm)

    def precalc_volume(self) -> None:
        lib.MPM_precalc_volume(self._mpm)

    def step(self) -> None:
        lib.MPM_step(self._mpm)

        self.profiler.step = self._mpm.profiler.step
        self.profiler.clear_grid = self._mpm.profiler.clear_grid
        self.profiler.p2g0 = self._mpm.profiler.p2g0
        self.profiler.p2g1 = self._mpm.profiler.p2g1
        self.profiler.update_grid = self._mpm.profiler.update_grid
        self.profiler.g2p = self._mpm.profiler.g2p

    def apply_brush(self, position: Vector2, radius: float, rel: Vector2) -> None:
        lib.MPM_apply_brush(self._mpm, (position.x, position.y), radius, (rel.x, rel.y))

    def get_particle_view(self, zoom: float, width: int, height: int) -> bytes:
        target = bytes([0 for _ in range(self.n_particles * 4 * 2)])

        lib.MPM_get_particle_view(self._mpm, target, zoom, width, height)

        return target