#include "global_buffer/global_buffer.h"
#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <string.h>
#include <math.h>

GlobalBuffer global_buffer;

void buffer_init() {
    mutex_init(&global_buffer.buffer_mutex);
    
    // Инициализация буферов
    memset(global_buffer.adc_buffers, 0, sizeof(global_buffer.adc_buffers));
    
    // Инициализация указателей
    global_buffer.write_buffer = 0;
    global_buffer.read_buffer = 0;
    global_buffer.processing_buffer = 0;
    
    // Флаги готовности
    for (int i = 0; i < NUM_BUFFERS; i++) {
        global_buffer.buffer_ready[i] = false;
    }
    
    // Настройки по умолчанию
    global_buffer.sample_rate = 500000; // 500 kHz
    global_buffer.trigger_level = 2048;  // Среднее значение (1.65V)
    global_buffer.trigger_enabled = true;
    global_buffer.trigger_edge = true;   // По фронту
    
    // Масштабирование
    global_buffer.time_scale = 1.0f;    // 1ms/div
    global_buffer.voltage_scale = 1.0f; // 1V/div
    global_buffer.voltage_offset = 0.0f;
    
    // Измерения
    global_buffer.max_value = 0;
    global_buffer.min_value = 4095;
    global_buffer.vpp = 0;
    global_buffer.frequency = 0;
    global_buffer.duty_cycle = 0.0f;
    
    // Состояние
    global_buffer.hold = false;
    global_buffer.running = false;
}

void buffer_swap() {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    
    // Находим следующий готовый буфер для обработки
    uint8_t next_buffer = (global_buffer.processing_buffer + 1) % NUM_BUFFERS;
    while (next_buffer != global_buffer.processing_buffer) {
        if (global_buffer.buffer_ready[next_buffer]) {
            global_buffer.processing_buffer = next_buffer;
            break;
        }
        next_buffer = (next_buffer + 1) % NUM_BUFFERS;
    }
    
    mutex_exit(&global_buffer.buffer_mutex);
}

void buffer_process() {
    if (!global_buffer.buffer_ready[global_buffer.processing_buffer]) {
        return;
    }
    
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    
    // Проверка триггера
    bool trigger_ok = !global_buffer.trigger_enabled || 
                     check_trigger(global_buffer.adc_buffers[global_buffer.processing_buffer]);
    
    if (trigger_ok) {
        // Обновление статистики
        buffer_update_stats(global_buffer.adc_buffers[global_buffer.processing_buffer]);
        
        // Если не в режиме удержания, обновляем буфер для отображения
        if (!global_buffer.hold) {
            global_buffer.read_buffer = global_buffer.processing_buffer;
        }
    }
    
    global_buffer.buffer_ready[global_buffer.processing_buffer] = false;
    mutex_exit(&global_buffer.buffer_mutex);
}

    void buffer_update_stats(uint16_t* buffer) {
    uint16_t min_val = 4095;
    uint16_t max_val = 0;
    uint32_t sum = 0;
    int zero_crossings = 0;
    bool last_state = buffer[0] > global_buffer.trigger_level;
    uint32_t high_samples = 0;
    
    // Первый проход: базовые измерения
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] < min_val) min_val = buffer[i];
        if (buffer[i] > max_val) max_val = buffer[i];
        sum += buffer[i];
        
        // Подсчёт пересечений уровня триггера
        bool current_state = buffer[i] > global_buffer.trigger_level;
        if (current_state != last_state) {
            zero_crossings++;
            last_state = current_state;
        }
        
        // Подсчёт времени высокого уровня для скважности
        if (current_state) high_samples++;
    }
    
    // Сохранение базовых измерений
    global_buffer.min_value = min_val;
    global_buffer.max_value = max_val;
    global_buffer.vpp = max_val - min_val;
    
    // Расчёт частоты и скважности (если есть достаточное количество пересечений)
    if (zero_crossings >= 2) {
        float avg_period = (BUFFER_SIZE * 2.0f) / zero_crossings;
        global_buffer.frequency = (uint16_t)(global_buffer.sample_rate / avg_period);
        global_buffer.duty_cycle = (high_samples * 100.0f) / BUFFER_SIZE;
    } else {
        global_buffer.frequency = 0;
        global_buffer.duty_cycle = 0.0f;
    }
    
    // Дополнительная обработка для более точных измерений
    if (global_buffer.frequency > 0) {
        // Уточнение периода сигнала
        int first_cross = -1, last_cross = -1;
        int crosses = 0;
        
        for (int i = 1; i < BUFFER_SIZE; i++) {
            bool prev_state = buffer[i-1] > global_buffer.trigger_level;
            bool curr_state = buffer[i] > global_buffer.trigger_level;
            
            if (prev_state != curr_state) {
                if (first_cross == -1) first_cross = i;
                last_cross = i;
                crosses++;
            }
        }
        
        if (crosses >= 2) {
            float exact_period = (last_cross - first_cross) * (crosses / (crosses - 1));
            global_buffer.frequency = (uint16_t)(global_buffer.sample_rate / exact_period);
        }
    }
}

bool check_trigger(uint16_t* buffer) {
    if (!global_buffer.trigger_enabled) return true;
    
    for (int i = 1; i < BUFFER_SIZE; i++) {
        if (global_buffer.trigger_edge) {
            // Rising edge trigger
            if (buffer[i-1] < global_buffer.trigger_level && 
                buffer[i] >= global_buffer.trigger_level) {
                // Найден фронт, можно сдвинуть буфер для точного позиционирования
                if (i > 50) { // Оставляем немного места перед фронтом
                    memmove(buffer, &buffer[i-50], (BUFFER_SIZE - (i-50)) * sizeof(uint16_t));
                }
                return true;
            }
        } else {
            // Falling edge trigger
            if (buffer[i-1] > global_buffer.trigger_level && 
                buffer[i] <= global_buffer.trigger_level) {
                if (i > 50) {
                    memmove(buffer, &buffer[i-50], (BUFFER_SIZE - (i-50)) * sizeof(uint16_t));
                }
                return true;
            }
        }
    }
    return false;
}

void buffer_set_sample_rate(uint32_t rate) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.sample_rate = rate;
    mutex_exit(&global_buffer.buffer_mutex);
}

void buffer_set_trigger(uint16_t level, bool enabled, bool edge) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.trigger_level = level;
    global_buffer.trigger_enabled = enabled;
    global_buffer.trigger_edge = edge;
    mutex_exit(&global_buffer.buffer_mutex);
}

void buffer_set_scale(float time_scale, float voltage_scale) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.time_scale = time_scale;
    global_buffer.voltage_scale = voltage_scale;
    mutex_exit(&global_buffer.buffer_mutex);
}

void buffer_set_hold(bool hold) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.hold = hold;
    mutex_exit(&global_buffer.buffer_mutex);
}