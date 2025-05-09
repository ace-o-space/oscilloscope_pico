#pragma once
#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <pico/sync.h>

#define BUFFER_SIZE 320
#define NUM_BUFFERS 2

typedef struct {
    // uint16_t adc_buffers[NUM_BUFFERS][BUFFER_SIZE];
    // volatile uint8_t write_buffer;
    volatile uint8_t read_buffer;
    // volatile uint8_t processing_buffer;
    volatile bool buffer_ready[NUM_BUFFERS];
    uint16_t adc_buffers[NUM_BUFFERS][BUFFER_SIZE];
    volatile uint8_t write_buffer;  // Куда пишет ADC
    volatile uint8_t display_buffer; // Что показываем на экране
    volatile uint8_t processing_buffer; // Что обрабатываем
    
    // Добавляем флаг для "живого" обновления
    volatile bool live_update;

    mutex_t buffer_mutex;
    
    // Настройки
    uint32_t sample_rate;
    uint16_t trigger_level;
    bool trigger_enabled;
    bool trigger_edge; // 0 - falling, 1 - rising
    
    // Измерения
    uint16_t max_value;
    uint16_t min_value;
    uint16_t vpp;
    uint16_t frequency;
    float duty_cycle;
    
    // Масштабирование
    float time_scale;
    float voltage_scale;
    float voltage_offset;
    
    // Состояние
    bool hold;
    bool running;

} GlobalBuffer;

extern GlobalBuffer global_buffer;

uint16_t* buffer_get_current();
void buffer_init(); 
void buffer_init();
void buffer_swap();
void buffer_process();
void buffer_update_stats();
bool check_trigger(uint16_t* buffer);