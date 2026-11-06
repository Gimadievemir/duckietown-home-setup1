// Учебная модель L298N + DC-мотор для Wokwi (custom chip).
// В Wokwi нет штатных деталей L298N и DC-мотора, поэтому модель написана
// по таблице истинности из даташита L298 (ST):
//   ENA = L            -> свободный выбег (COAST)
//   ENA = H, IN1 = IN2 -> быстрая остановка (BRAKE)
//   ENA = H, IN1 > IN2 -> вперёд (FORWARD)
//   ENA = H, IN1 < IN2 -> назад (REVERSE)
// Модель логическая: ток, нагрев, просадку Vs, ЭДС самоиндукции и инерцию
// ротора НЕ моделирует. Скорость на экране = коэффициент заполнения ENA.

#include "wokwi-api.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { COAST, FORWARD, REVERSE, BRAKE };
static const char *MODE_NAME[] = { "COAST(stop)", "FORWARD", "REVERSE", "BRAKE(stop)" };

typedef struct {
  pin_t in1, in2, ena, out1, out2, echo;
  buffer_t fb;
  uint32_t w, h;
  uint8_t *pix;

  uint64_t ena_rise_ns, ena_period_ns, ena_high_ns, ena_last_edge_ns;
  uint64_t last_echo_ns;

  int mode, duty_pct;
  uint32_t freq_hz;
  int logged_mode, logged_duty, logged_in1, logged_in2;
  uint32_t logged_freq;
  float angle;
} chip_t;

static void update_outputs(chip_t *c) {
  uint32_t en = pin_read(c->ena);
  pin_write(c->out1, en && pin_read(c->in1));
  pin_write(c->out2, en && pin_read(c->in2));
}

static void on_ena(void *user_data, pin_t pin, uint32_t value) {
  chip_t *c = user_data;
  uint64_t now = get_sim_nanos();
  if (value) {
    if (c->ena_rise_ns) c->ena_period_ns = now - c->ena_rise_ns;
    c->ena_rise_ns = now;
  } else if (c->ena_rise_ns) {
    c->ena_high_ns = now - c->ena_rise_ns;
  }
  c->ena_last_edge_ns = now;
  update_outputs(c);
}

static void on_in(void *user_data, pin_t pin, uint32_t value) {
  update_outputs(user_data);
}

static void on_echo(void *user_data, pin_t pin, uint32_t value) {
  chip_t *c = user_data;
  if (!value) c->last_echo_ns = get_sim_nanos();
}

static void put(chip_t *c, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (x < 0 || y < 0 || x >= (int)c->w || y >= (int)c->h) return;
  uint8_t *p = c->pix + (y * c->w + x) * 4;
  p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
}

static void draw(chip_t *c) {
  static const uint8_t color[4][3] = { {120, 120, 120}, {40, 200, 70}, {240, 150, 30}, {220, 40, 40} };
  const uint8_t *col = color[c->mode];
  int cx = c->w / 2, cy = c->h / 2 - 6, r = 20;

  memset(c->pix, 0, c->w * c->h * 4);
  for (uint32_t i = 0; i < c->w * c->h; i++) c->pix[i * 4 + 3] = 255;

  for (int a = 0; a < 360; a += 2) {            // корпус мотора
    float t = a * 3.14159f / 180;
    put(c, cx + (int)(r * cosf(t)), cy + (int)(r * sinf(t)), col[0], col[1], col[2]);
  }
  for (int k = 0; k < 2; k++) {                  // две лопасти ротора
    float t = c->angle + k * 3.14159f;
    for (int s = 0; s < r - 3; s++)
      put(c, cx + (int)(s * cosf(t)), cy + (int)(s * sinf(t)), 230, 230, 230);
  }
  int bar = (c->mode == FORWARD || c->mode == REVERSE) ? c->duty_pct * (c->w - 8) / 100 : 0;
  for (int x = 4; x < (int)c->w - 4; x++)        // шкала скорости = duty ENA
    for (int y = c->h - 10; y < (int)c->h - 4; y++)
      put(c, x, y, x - 4 < bar ? col[0] : 40, x - 4 < bar ? col[1] : 40, x - 4 < bar ? col[2] : 40);

  buffer_write(c->fb, 0, c->pix, c->w * c->h * 4);
}

static void on_tick(void *user_data) {
  chip_t *c = user_data;
  uint64_t now = get_sim_nanos();

  if (c->ena_period_ns && now - c->ena_last_edge_ns < 20000000ULL) {
    c->duty_pct = (int)(c->ena_high_ns * 100 / c->ena_period_ns);
    c->freq_hz = (uint32_t)(1000000000ULL / c->ena_period_ns);
  } else {                                       // нет фронтов 20 мс: постоянный уровень
    c->duty_pct = pin_read(c->ena) ? 100 : 0;
    c->freq_hz = 0;
    c->ena_period_ns = 0;
    c->ena_rise_ns = 0;
  }

  int in1 = pin_read(c->in1), in2 = pin_read(c->in2);
  if (c->duty_pct == 0) c->mode = COAST;
  else if (in1 == in2) c->mode = BRAKE;
  else c->mode = in1 ? FORWARD : REVERSE;

  if (c->mode == FORWARD) c->angle += c->duty_pct * 0.004f;
  if (c->mode == REVERSE) c->angle -= c->duty_pct * 0.004f;
  draw(c);

  int duty_changed = abs(c->duty_pct - c->logged_duty) > 2;
  if (c->mode != c->logged_mode || duty_changed || in1 != c->logged_in1 || in2 != c->logged_in2 ||
      (c->freq_hz != 0) != (c->logged_freq != 0)) {
    printf("[L298N t=%.3fs] IN1=%d IN2=%d ENA duty=%d%% f=%uHz -> %s",
           now / 1e9, in1, in2, c->duty_pct, c->freq_hz, MODE_NAME[c->mode]);
    if ((c->mode == COAST || c->mode == BRAKE) && c->last_echo_ns)
      printf(" (%.0f ms after last ECHO pulse)", (now - c->last_echo_ns) / 1e6);
    printf("\n");
    c->logged_mode = c->mode;
    c->logged_duty = c->duty_pct;
    c->logged_in1 = in1;
    c->logged_in2 = in2;
    c->logged_freq = c->freq_hz;
  }
}

void chip_init(void) {
  chip_t *c = calloc(1, sizeof(chip_t));
  c->in1 = pin_init("IN1", INPUT);
  c->in2 = pin_init("IN2", INPUT);
  c->ena = pin_init("ENA", INPUT);
  c->echo = pin_init("ECHO", INPUT);
  c->out1 = pin_init("OUT1", OUTPUT_LOW);
  c->out2 = pin_init("OUT2", OUTPUT_LOW);
  c->logged_mode = -1;

  const pin_watch_config_t ena_watch = { .edge = BOTH, .pin_change = on_ena, .user_data = c };
  const pin_watch_config_t in_watch = { .edge = BOTH, .pin_change = on_in, .user_data = c };
  const pin_watch_config_t echo_watch = { .edge = BOTH, .pin_change = on_echo, .user_data = c };
  pin_watch(c->ena, &ena_watch);
  pin_watch(c->in1, &in_watch);
  pin_watch(c->in2, &in_watch);
  pin_watch(c->echo, &echo_watch);

  c->fb = framebuffer_init(&c->w, &c->h);
  c->pix = malloc(c->w * c->h * 4);

  const timer_config_t tick = { .callback = on_tick, .user_data = c };
  timer_start(timer_init(&tick), 10000, true);  // 10 мс времени симуляции
  printf("[L298N] model started, display %ux%u\n", c->w, c->h);
}
