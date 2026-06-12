from array import array

import pygame
import moderngl


def create_buffer_object(
        ctx: moderngl.Context,
        data: list[float | int]
        ) -> moderngl.Buffer:
    """ Create buffer object from array. """

    dtype = "f" if isinstance(data[0], float) else "I"
    return ctx.buffer(array(dtype, data))


class Quad:
    """ Base textured quad. """

    def __init__(self,
            ctx: moderngl.Context,
            vertex_shader: str = "base.vsh",
            fragment_shader: str = "base.fsh"
            ) -> None:
        self._ctx = ctx

        def_surf = pygame.Surface((2, 2))
        def_surf.fill((255, 0, 255))
        self.load_texture(def_surf)

        self._vbo = create_buffer_object(self._ctx, [-1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0])
        self._uvbo = create_buffer_object(self._ctx, [0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0])
        self._ibo = create_buffer_object(self._ctx, [0, 1, 2, 1, 2, 3])

        self.load_program(vertex_shader, fragment_shader)

        self.vao = self._ctx.vertex_array(
            self.program,
            (
                (self._vbo, "2f", "in_position"),
                (self._uvbo, "2f", "in_uv")
            ),
            self._ibo
        )
    
    def load_program(self, vertex_shader: str, fragment_shader: str) -> None:
        """ Load shader programs from shader sources. """

        with open(vertex_shader, "r", encoding="utf-8") as file:
            vsh_source = file.read()

        with open(fragment_shader, "r", encoding="utf-8") as file:
            fsh_source = file.read()

        self.program = self._ctx.program(
            vertex_shader=vsh_source, fragment_shader=fsh_source
        )

        self.program["s_texture"] = 0

    def load_texture(self, surf: pygame.Surface) -> None:
        """ Create new texture from surface. """
        self.texture = self._ctx.texture(surf.size, 4)
        self.update_texture(surf)

    def update_texture(self, surf: pygame.Surface) -> None:
        """ Update the texture from surface. """
        self.texture.write(surf.get_view("1"))

    def get_surface(self) -> pygame.Surface:
        """ Get surface from texture data. """
        return pygame.image.frombytes(
            self.texture.read(), self.texture.size, "RGBA", flipped=True
        )
    
    def render(self) -> None:
        self.texture.use(0)
        self.vao.render()


class Particles:
    def __init__(self,
            ctx: moderngl.Context,
            vertex_shader: str = "base.vsh",
            fragment_shader: str = "base.fsh"
            ) -> None:
        self._ctx = ctx

        # max_particles * sizeof(float) * 2
        max_particles = 100_000
        self.pos_buffer = ctx.buffer(reserve=max_particles * 4 * 2, dynamic=True)
        self.vel_buffer = ctx.buffer(reserve=max_particles * 4 * 2, dynamic=True)
        self.mat_buffer = ctx.buffer(reserve=max_particles * 4, dynamic=True)

        self.load_program(vertex_shader, fragment_shader)

        self.vao = self._ctx.vertex_array(
            self.program,
            (
                (self.pos_buffer, "2f", "in_position"),
                (self.vel_buffer, "2f", "in_velocity"),
                (self.mat_buffer, "1u", "in_material")
            )
        )

    def load_program(self, vertex_shader: str, fragment_shader: str) -> None:
        """ Load shader programs from shader sources. """

        with open(vertex_shader, "r", encoding="utf-8") as file:
            vsh_source = file.read()

        with open(fragment_shader, "r", encoding="utf-8") as file:
            fsh_source = file.read()

        self.program = self._ctx.program(
            vertex_shader=vsh_source, fragment_shader=fsh_source
        )

    def render(self, n_particles: int) -> None:
        self.vao.render(mode=moderngl.POINTS, vertices=n_particles)