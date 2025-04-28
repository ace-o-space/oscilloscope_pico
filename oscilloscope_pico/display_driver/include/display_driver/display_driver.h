#pragma once
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

// Константы для кнопок
#define BUTTON_HOLD 2
#define BUTTON_PLUS 3
#define BUTTON_MINUS 4
#define BUTTON_SET 5

// Состояния меню
typedef enum {
    MENU_NONE,
    MENU_TIME_SCALE,
    MENU_VOLT_SCALE,
    MENU_TRIGGER
} MenuState;

// Инициализация
void display_init(void);
void init_buttons(void);

// Основной цикл
void display_task(void);

// Обработка ввода
void process_buttons(void);

// Функции отрисовки
void draw_grid(void);
void draw_waveform(void);
void draw_measurements(void);
