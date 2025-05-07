#pragma once
#include <stdint.h>
#include <stdbool.h>

void adc_start(void);
bool adc_get_buffer(uint16_t** buffer);  // Возвращает успех и записывает указатель
// Инициализация подсистемы АЦП
void adc_processor_init();

// Основная задача обработки АЦП (запускается на ядре 1)
void core0_adc_task();

// Функции для управления параметрами захвата
void set_sample_rate(uint32_t rate_khz);
void set_trigger_level(uint16_t level);
void enable_trigger(bool enable);

// Вспомогательные функции
uint16_t get_last_sample();
uint32_t get_current_sample_rate();

// Структура конфигурации АЦП
typedef struct {
    uint8_t input_channel;
    uint8_t sample_bits;
    bool use_dma;
    uint32_t target_sample_rate;
} ADCConfig;