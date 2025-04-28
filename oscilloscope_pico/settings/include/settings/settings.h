#pragma once
#include "pico/stdlib.h"

// Настройки временной развертки
typedef enum {
    TIMEBASE_1US,
    TIMEBASE_10US,
    TIMEBASE_100US,
    TIMEBASE_1MS,
    TIMEBASE_10MS,
    TIMEBASE_100MS
} TimebaseSetting;

// Настройки вольт/деление
typedef enum {
    VOLT_DIV_0_1V,
    VOLT_DIV_0_2V,
    VOLT_DIV_0_5V,
    VOLT_DIV_1V,
    VOLT_DIV_2V,
    VOLT_DIV_5V
} VoltDivSetting;

// Настройки синхронизации
typedef enum {
    TRIGGER_AUTO,
    TRIGGER_NORMAL,
    TRIGGER_SINGLE
} TriggerMode;

typedef enum {
    EDGE_RISING,
    EDGE_FALLING,
    EDGE_BOTH
} TriggerEdge;

// Полная конфигурация осциллографа
typedef struct {
    TimebaseSetting timebase;
    VoltDivSetting volt_div;
    TriggerMode trigger_mode;
    TriggerEdge trigger_edge;
    uint16_t trigger_level;
    bool persistence;
    bool measurements_enabled;
    bool cursors_enabled;
} OscilloscopeSettings;

// Функции для работы с настройками
void settings_init();
void settings_save();
void settings_load();
void settings_reset();

// Геттеры/сеттеры для отдельных параметров
void set_timebase(TimebaseSetting tb);
TimebaseSetting get_timebase();
float get_timebase_us();

void set_volt_div(VoltDivSetting vd);
VoltDivSetting get_volt_div();
float get_volt_div_value();