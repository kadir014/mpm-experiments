import pygame


class BrushEngine:
    def __init__(self) -> None:
        self._started = False
        self._trail: list[pygame.Vector2] = []

        self.min_dist: float = 3.0

    @property
    def raw_trail(self) -> list[pygame.Vector2]:
        return self._trail.copy()

    def down(self) -> None:
        self._started = True

    def up(self) -> None:
        self._started = False
        self._trail.clear()

    def update(self) -> None:
        if not self._started:
            return

        now = pygame.Vector2(*pygame.mouse.get_pos())

        if len(self._trail) > 0:
            last = self._trail[-1]
            if last.distance_squared_to(now) < self.min_dist * self.min_dist:
                return

        self._trail.append(now)


def draw_capsule(
        surface: pygame.Surface,
        color: pygame.typing.ColorLike,
        start: pygame.typing.Point,
        end: pygame.typing.Point,
        radius: float,
        width: int = 0,
        quality: int = 4
        ) -> None:
    start = pygame.Vector2(start)
    end = pygame.Vector2(end)
    
    local_arch = []
    arm = pygame.Vector2(radius, 0.0)
    local_angle = 180 / (quality + 1)
    for _ in range(quality+2):
        local_arch.append(arm)
        arm = arm.rotate(local_angle)

    delta = end - start
    dir = delta.normalize()
    angle = dir.angle

    poly = []
    start_arch = [v.rotate(angle + 90) + start for v in local_arch]
    poly += start_arch

    end_arch = [v.rotate(angle - 90) + end for v in local_arch]
    poly += end_arch

    pygame.draw.polygon(surface, color, poly, width)

def draw_stroke(
        surface: pygame.Surface,
        color: pygame.typing.ColorLike,
        points: list[pygame.typing.Point],
        radius: float
        ) -> None:
    for i in range(len(points) - 1):
        a = points[i]
        b = points[i + 1]
        draw_capsule(surface, color, a, b, radius, 0, quality=5)