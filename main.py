import os
import struct
from random import uniform
from math import ceil, sin

import pygame
import moderngl
import miniprofiler

import mpm
import brush
from quad import Quad, Particles


WINDOW_SIZE = WINDOW_WIDTH, WINDOW_HEIGHT = 1280, 720
MAX_FPS = 60
ZOOM = 7.2

BRUSH_RADIUS = 3.5 * 3
DRAW_RADIUS = 30


# Thanks to DD for desktop scaling fix on linux
if os.environ.get("XDG_SESSION_TYPE", "").strip().lower() == "wayland":
    os.environ["SDL_VIDEODRIVER"] = "wayland"

pygame.init()
window = pygame.display.set_mode(WINDOW_SIZE, flags=pygame.OPENGL | pygame.DOUBLEBUF)
clock = pygame.Clock()
is_running = True
font = pygame.Font("assets/MartianMonoNerdFont-Regular.ttf", 11)
prof = miniprofiler.Profiler(60)
with prof.profile("render"): ...

ctx = moderngl.get_context()
ctx.enable(moderngl.BLEND)
ctx.enable_direct(0x8861)

particle_surf = pygame.Surface((4, 4))
particle_surf.fill((0, 200, 255))

particle_surf2 = particle_surf.copy()
particle_surf2.fill((115, 227, 64))

screenquad = Quad(ctx, "shaders/base.vsh", "shaders/final.fsh")
screenquad.load_texture(window)

particles_gl = Particles(ctx, "shaders/particles.vsh", "shaders/particles.fsh")


sim = mpm.MPM((100, 100), hertz=40.0, substeps=6)
#sim.gravity = mpm.Vector2(0.0)
#sim.gravity = mpm.Vector2(0.0, 3.0)
sim.gravity = mpm.Vector2(0.0, 9.81) * 2.3

def spawn(sp: pygame.Vector2, size: tuple[int, int], e: float = 15000.0, mass: float = 1.0, material: int = 1) -> None:
    for y in range(size[1]):
        # if (y+0) % 6 < 3:
        #     continue
        for x in range(size[0]):
            # if x % 10 > 3:
            #     continue
            # if sin(x+y) < 0.5:
            #     continue
            p = pygame.Vector2(x, y) * 0.75 + sp
            r = 0.1
            #p += pygame.Vector2(uniform(-r, r), uniform(-r, r))
            v = pygame.Vector2(uniform(-1, 1), uniform(-1, 1)) * 40
            v = pygame.Vector2()
            # if uniform(0, 1) <= 0.5:
            #     e=10000
            # else:
            #     e=10
            
            if material == 1:
                #sim.add_fluid_particle(p, v, mass=mass*0.3, rest_density=0.3, gravity_scale=-0.6)

                sim.add_fluid_particle(p, v, mass=mass*1.0)
            else:
                sim.add_elastic_particle(p, v, mass=mass*1.0)

    if material == 0:
        sim.precalc_volume()

spawn(pygame.Vector2(3.0, 3.0), (45, 123), material=1)
spawn(pygame.Vector2(65.0, 64), (5, 45), material=0)

def point_segment_distance(
        p: pygame.Vector2,
        a: pygame.Vector2,
        b: pygame.Vector2
        ) -> float:
    ab = b - a
    ap = p - a

    ab_len_sq = ab.dot(ab)

    # Handle degenerate segment
    if ab_len_sq == 0:
        return (p - a).length()

    t = ap.dot(ab) / ab_len_sq
    t = max(0.0, min(1.0, t))

    closest = a + ab * t

    return (p - closest).length()


brushengine = brush.BrushEngine()
paused = False

drawsurf = pygame.Surface((100 * ZOOM, 100*ZOOM))

frame_n = 0

while is_running:
    dt = clock.tick(MAX_FPS) * 0.001

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            is_running = False

        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                is_running = False

            elif event.key == pygame.K_SPACE:
                paused = not paused

            elif event.key == pygame.K_c:
                sim.clear()

            elif event.key == pygame.K_F1:
                max_mass = -float("inf")
                for y in range(100):
                    for x in range(100):
                        cell = sim.cell(x, y)
                        mass = cell[1]
                        v = pygame.Vector2(cell[0].to_tuple())
                        if mass > max_mass:
                            max_mass = mass
                print("max mass:", max_mass)
        
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                brushengine.down()

        elif event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1:
                trail = brushengine.raw_trail
                brushengine.up()

                res = 1.5
                for y in range(round(100 * res)):
                    y /= res
                    for x in range(round(100 * res)):
                        x /= res
                        p = pygame.Vector2(x, y) * (ZOOM)

                        c = drawsurf.get_at(p)

                        if c.r == 0:
                            r = 0.1
                            ppos = mpm.Vector2(x, y)
                            ppos += pygame.Vector2(uniform(-r, r), uniform(-r, r))
                            sim.add_elastic_particle(ppos, mass=1.0, elastic_lambda=5000, elastic_mu=15000)

                        # for i in range(len(trail)-1):
                        #     a = trail[i]
                        #     b = trail[i + 1]
                        #     if point_segment_distance(p, a, b) <= DRAW_RADIUS:
                        #         r = 0.1
                        #         ppos = mpm.Vector2(x, y)
                        #         ppos += pygame.Vector2(uniform(-r, r), uniform(-r, r))
                        #         sim.add_elastic_particle(ppos, mass=1.0, elastic_lambda=5000, elastic_mu=25000)
                        #         break

                sim.precalc_volume()

    keys = pygame.key.get_pressed()
    mouse = pygame.Vector2(*pygame.mouse.get_pos())
    pmouse = mouse / ZOOM
    pmouse_rel = pygame.Vector2(*pygame.mouse.get_rel()) / ZOOM

    if keys[pygame.K_q]:
        if frame_n >= 7:
            frame_n = 0
            spawn(pmouse-pygame.Vector2(5, 5), (10, 10), mass=1.0)

    brushengine.update()

    if not paused:
        with prof.profile("step"):
            sim.step()

            if pygame.mouse.get_pressed()[2]:
                sim.apply_brush(pmouse, BRUSH_RADIUS, pmouse_rel)

    with prof.profile("render"):
        window.fill((255, 255, 255))
        drawsurf.fill((255, 255, 255))

        # for y in range(100):
        #     for x in range(100):
        #         cell = sim.cell(x, y)
        #         mass = cell[1]
        #         vel = pygame.Vector2(cell[0].to_tuple())

        #         if mass > 0:
        #             l = pygame.math.clamp(mass * 55, 0, 50)
        #             h = (int(mass * 25.0) - 100) % 360
        #             #h = int(vel.length() / 1.0) % 360
        #             color = pygame.Color.from_hsla((h, 100, 100 - l, 100))

        #             window.fill(color, (int(x*ZOOM), int(y*ZOOM), ceil(ZOOM), ceil(ZOOM)))

        # for y in range(100 + 1):
        #     pygame.draw.line(window, (240, 240, 240), (0, y * ZOOM), (100 * ZOOM, y * ZOOM), 1)

        # for x in range(100 + 1):
        #     pygame.draw.line(window, (240, 240, 240), (x * ZOOM, 0), (x * ZOOM, 100 * ZOOM), 1)

        pygame.draw.rect(window, (240, 240, 240), (0, 0, 100 * ZOOM, 100 * ZOOM), 1)

        if pygame.mouse.get_pressed()[2]:
            pygame.draw.circle(window, (160, 255, 90), mouse, BRUSH_RADIUS * ZOOM, 1)

        # for p in sim.iter_particles():
        #     pos = p[0]
        #     cell = sim.cell(int(pos.x), int(pos.y))
        #     mass = cell[1]
        #     vel = pygame.Vector2(cell[0].to_tuple())

        #     l = pygame.math.clamp(mass * 55, 0, 50)
        #     #h = (int(mass * 25.0) - 100) % 360
        #     h = int(vel.length() / 1.0) % 360
        #     color = pygame.Color.from_hsla((h, 100, 100 - l, 100))

        #     pygame.draw.circle(window, color, (p[0] * ZOOM).to_tuple(), 3)

        #fblits_seq = [((particle_surf2, particle_surf)[p[2] == 1], (p[0] * ZOOM).to_tuple()) for p in sim.iter_particles()]
        #window.fblits(fblits_seq)

        brush.draw_stroke(window, (0, 0, 0), brushengine.raw_trail, DRAW_RADIUS)
        brush.draw_stroke(drawsurf, (0, 0, 0), brushengine.raw_trail, DRAW_RADIUS)
        if pygame.mouse.get_pressed()[0]:
            pygame.draw.circle(window, (160, 255, 90), mouse, DRAW_RADIUS, 1)
            
        lines = (
            f"Particles:  {sim.n_particles}/{sim.max_particles}",
            f"FPS:        {round(clock.get_fps(), 1)}",
            f"Sim Speed:  {round(clock.get_fps() / 40.0, 3)}x",
            f"Sim Hertz:  {round(sim.hertz)} (x{sim.substeps} substeps = {round(sim.hertz * sim.substeps)}Hz)",
            "",
             "        avg        %5         %95",
            f"Render: {round(prof['render'].avg * 1000.0, 3): <10} {round(prof['render'].p05 * 1000.0, 3): <10} {round(prof['render'].p95 * 1000.0, 3): <10} ms",
            f"Step:   {round(prof['step'].avg * 1000.0, 3): <10} {round(prof['step'].p05 * 1000.0, 3): <10} {round(prof['step'].p95 * 1000.0, 3): <10} ms",
            f"Clear:  {round(sim.profiler.clear_grid * 1000.0, 3)} ms",
            f"P2G 0:  {round(sim.profiler.p2g0 * 1000.0, 3)} ms",
            f"P2G 1:  {round(sim.profiler.p2g1 * 1000.0, 3)} ms",
            f"Update: {round(sim.profiler.update_grid * 1000.0, 3)} ms",
            f"G2P:    {round(sim.profiler.g2p * 1000.0, 3)} ms",
        )
        for i, line in enumerate(lines):
            y = i * 18 + 10
            window.blit(font.render(line, True, (0, 0, 0)), (750, y))

        ctx.clear(0, 0, 0)

        screenquad.update_texture(window)
        screenquad.render()

        particles_view = sim.get_particle_view(ZOOM, WINDOW_WIDTH, WINDOW_HEIGHT)
        particles_gl.pos_buffer.write(particles_view[0])
        particles_gl.vel_buffer.write(particles_view[1])
        particles_gl.mat_buffer.write(particles_view[2])

        ctx.point_size = 7
        particles_gl.render(sim.n_particles)

        pygame.display.flip()

    pygame.display.set_caption(f"MPM playground @ {round(clock.get_fps())} FPS")
    frame_n += 1

pygame.quit()