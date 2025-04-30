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

void get_values_for_draw(uint16_t*, float*, float*);

// Функции отрисовки
void draw_grid(void);
void draw_measurements(float*);
void draw_waveform(uint16_t*, float*);

//функции стирания 
void erase_grid(void);
void erase_measurements(float*);
void erase_waveform(uint16_t*, float*);  

