import pygame


class Slider:
    def __init__(self,
            font: pygame.Font,
            position: pygame.Vector2,
            text: str = "",
            length: float = 100.0,
            min_value: float = 0.0,
            max_value: float = 1.0,
            value: float | None = None,
            display_integer: bool = False,
            formatter: str = "{value}"
            ) -> None:
        self.font = font
        self.position = position
        self.text = text
        self.length = length
        self.height = 4
        self.display_integer = display_integer
        self.formatter = formatter

        self.min_value = min_value
        self.max_value = max_value
        self.value = value if value is not None else self.min_value
        self.normalized = (self.value - self.min_value) / (self.max_value - self.min_value)

        # For interaction in the game loop
        self.changed = False

        self._hovered = False
        self._pressed = False
        self._pressed_pos = pygame.Vector2()
        self._sensitive = False

    @property
    def value_int(self) -> int:
        return round(self.value)

    def update(self) -> None:
        self.changed = False

        mouse = pygame.Vector2(*pygame.mouse.get_pos())
        collision_pad = 15
        bar = pygame.FRect(self.position, (self.length, self.height))
        bar.inflate_ip(0, collision_pad)

        if bar.collidepoint(mouse):
            self._hovered = True
        else:
            self._hovered = False

        if self._hovered and pygame.mouse.get_just_pressed()[0]:
            self._pressed = True
            self._pressed_pos = mouse.copy()
        
        elif pygame.mouse.get_just_released()[0]:
            self._pressed = False

        self._sensitive = False
        if pygame.key.get_pressed()[pygame.K_LSHIFT]:
            self._sensitive = True

        if self._pressed:
            if self._sensitive:
                delta = mouse - self._pressed_pos
                self.normalized += delta.x * 0.0009
                self.normalized = pygame.math.clamp(self.normalized, 0.0, 1.0)
                self._pressed_pos = mouse.copy()

            else:
                x = mouse.x - self.position.x
                self.normalized = pygame.math.clamp(x / self.length, 0.0, 1.0)

            value0 = self.value
            self.value = self.min_value + self.normalized * (self.max_value - self.min_value)

            if self.value != value0:
                self.changed = True

    def render(self, surface: pygame.Surface) -> None:
        accent = (134, 87, 255)

        pygame.draw.rect(surface, accent, (self.position, (self.length, self.height)), 0, border_radius=99)

        handle = self.position + pygame.Vector2(self.normalized * self.length, 4)
        handle.y = int(handle.y) - 2
        pygame.draw.circle(surface, (255, 255, 255), handle, 5)
        #pygame.draw.circle(surface, accent, handle, 7)
        pygame.draw.aacircle(surface, accent, handle, 7, 2)

        text_surf = self.font.render(self.text, True, (0, 0, 0))
        surface.blit(text_surf, self.position - pygame.Vector2(text_surf.width + 14, 6))

        prec = 5 if self._sensitive else 2
        display = self.formatter.format(value=round(self.value, prec))
        if self.display_integer:
            display = self.formatter.format(value=self.value_int)
        text_surf = self.font.render(display, True, accent)
        surface.blit(text_surf, self.position + pygame.Vector2(self.length + 14, -6))