from random import uniform

import pygame
import miniprofiler

import mpm


WINDOW_SIZE = WINDOW_WIDTH, WINDOW_HEIGHT = 1280, 720
MAX_FPS = 60
ZOOM = 7.2

BRUSH_RADIUS = 3.5 * 3


pygame.init()
window = pygame.display.set_mode(WINDOW_SIZE)
clock = pygame.Clock()
is_running = True
font = pygame.Font("assets/MartianMonoNerdFont-Regular.ttf", 12)
prof = miniprofiler.Profiler(60)
with prof.profile("render"): ...

particle_surf = pygame.Surface((4, 4))
particle_surf.fill((0, 200, 255))

particle_surf2 = particle_surf.copy()
particle_surf2.fill((115, 227, 64))


sim = mpm.MPM((100, 100), hertz=30.0, substeps=8)
#sim.gravity = mpm.Vector2(0.0)
#sim.gravity = mpm.Vector2(0.0, 0.3)

def spawn(size: tuple[int, int], e: float, mass: float = 1.0) -> None:
    for y in range(size[1]):
        # if (y+0) % 6 < 3:
        #     continue
        for x in range(size[0]):
            # if x % 10 < 5:
            #     continue
            if (x**y) % 10 < 5:
                continue
            p = pygame.Vector2(x+0.5, y+0.5) * 0.5 + pygame.Vector2(5, 5)
            r = 0.1
            #p += pygame.Vector2(uniform(-r, r), uniform(-r, r))
            v = pygame.Vector2(uniform(-1, 1), uniform(-1, 1)) * 500
            v = pygame.Vector2()
            # if uniform(0, 1) <= 0.5:
            #     e=10000
            # else:
            #     e=10
            sim.add_particle(p, v, mass=mass, elastic_mu=e)

    sim.precalc_volume()

spawn((115, 115), 15000.0, mass=1.0)


while is_running:
    dt = clock.tick(MAX_FPS) * 0.001

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            is_running = False

        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                is_running = False

            elif event.key == pygame.K_SPACE:
                spawn((45, 45), 10.0, 0.3)

    mouse = pygame.Vector2(*pygame.mouse.get_pos())
    pmouse = mouse / ZOOM
    pmouse_rel = pygame.Vector2(*pygame.mouse.get_rel()) / ZOOM

    with prof.profile("step"):
        sim.step()

        if pygame.mouse.get_pressed()[0]:
            sim.apply_brush(pmouse, BRUSH_RADIUS, pmouse_rel)

    with prof.profile("render"):
        window.fill((255, 255, 255))

        for y in range(100 + 1):
            pygame.draw.line(window, (240, 240, 240), (0, y * ZOOM), (100 * ZOOM, y * ZOOM), 1)

        for x in range(100 + 1):
            pygame.draw.line(window, (240, 240, 240), (x * ZOOM, 0), (x * ZOOM, 100 * ZOOM), 1)

        pygame.draw.circle(window, (160, 255, 90), mouse, BRUSH_RADIUS * ZOOM, 1)

        # for y in range(64):
        #     for x in range(64):
        #         cell = sim.cell(x, y)
        #         mass = cell[1]
        #         v = pygame.Vector2(cell[0].to_tuple())

        #         p = (pygame.Vector2(x, y) + pygame.Vector2(0.5, 0.5)) * ZOOM
        #         #pygame.draw.circle(window, (0, 255, 0), p, mass * ZOOM * 1, 1)
        #         pygame.draw.line(window, (255, 190, 180), p, p + v * ZOOM * 0.2, 1)

        #for p in sim.iter_particles():
           #pygame.draw.circle(window, (0, 200, 255), (p * ZOOM).to_tuple(), 3)
           #print(p)

        fblits_seq = [((particle_surf2, particle_surf)[p[1] == 10.0], (p[0] * ZOOM).to_tuple()) for p in sim.iter_particles()]
        window.fblits(fblits_seq)
            
        lines = (
            f"Particles:  {sim.n_particles}/{sim.max_particles}",
            f"FPS:        {round(clock.get_fps())}",
            f"Sim. Speed: {round(clock.get_fps() / 30.0, 3)}x",
            "",
             "        avg        %5         %95",
            f"Step:   {round(prof['step'].avg * 1000.0, 3): <10} {round(prof['step'].p05 * 1000.0, 3): <10} {round(prof['step'].p95 * 1000.0, 3): <10} ms",
            f"Render: {round(prof['render'].avg * 1000.0, 3): <10} {round(prof['render'].p05 * 1000.0, 3): <10} {round(prof['render'].p95 * 1000.0, 3): <10} ms",
        )
        for i, line in enumerate(lines):
            y = i * 20 + 10
            window.blit(font.render(line, True, (0, 0, 0)), (750, y))

        pygame.display.flip()

    pygame.display.set_caption(f"MPM playground @ {round(clock.get_fps())} FPS")

pygame.quit()