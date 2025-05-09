#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void core1_display_task(void);  // Правильное объявление для Cortex-M0+

#ifdef __cplusplus
}
#endif

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "pico_ili9341/pico_ili9341.h"

// Константы для кнопок
#define BUTTON_HOLD 2
#define BUTTON_PLUS 3
#define BUTTON_MINUS 4
#define BUTTON_SET 5

#define DISPLAY_WIDTH  ILI9341_WIDTH
#define DISPLAY_HEIGHT ILI9341_HEIGHT
#define WAVEFORM_WIDTH  ILI9341_HEIGHT
#define WAVEFORM_HEIGHT (DISPLAY_WIDTH - WAVEFORM_TOP)
#define WAVEFORM_TOP  50


// Состояния мен
typedef enum {
    MENU_NONE,
    MENU_TIME_SCALE,
    MENU_VOLT_SCALE,
    MENU_TRIGGER
} MenuState;

typedef struct {
    uint16_t min_y; // Минимальная Y-координата сигнала в столбце
    uint16_t max_y; // Максимальная Y-координата
} ColumnRange;

// Инициализация
void display_init(void);
void init_buttons(void);

// Обработка ввода
void process_buttons(void);

void get_values_for_draw(uint16_t *, float*, float *);

// Функции отрисовки
void draw_grid(void);
void draw_measurements(float *);
void draw_waveform(uint16_t*);


