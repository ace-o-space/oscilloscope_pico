#include "adc_driver/adc_driver.h"
#include "global_buffer/global_buffer.h"
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <math.h>

static int dma_chan;
static volatile bool adc_running = false;

extern void buffer_update_stats(uint16_t* buffer);
extern bool check_trigger(uint16_t* buffer);
extern void buffer_process();

void adc_dma_handler() {
    if (dma_channel_get_irq0_status(dma_chan)) {
        dma_channel_acknowledge_irq0(dma_chan);
        
        mutex_enter_blocking(&global_buffer.buffer_mutex);
        
        // Переключение буферов
        global_buffer.processing_buffer = global_buffer.write_buffer;
        global_buffer.buffer_ready[global_buffer.processing_buffer] = true;
        global_buffer.write_buffer = (global_buffer.write_buffer + 1) % NUM_BUFFERS;
        
        mutex_exit(&global_buffer.buffer_mutex);
        
        // Перезапуск DMA с новым буфером
        dma_channel_set_write_addr(dma_chan, 
            global_buffer.adc_buffers[global_buffer.write_buffer], true);
        dma_channel_set_trans_count(dma_chan, BUFFER_SIZE, true);
    }
}

void adc_processor_init() {
    adc_init();
    adc_gpio_init(26); // ADC0 на GPIO26
    adc_select_input(0);
    adc_fifo_setup(true, true, 1, false, false);
    
    // Настройка DMA
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
    
    dma_channel_configure(dma_chan, &cfg,
        global_buffer.adc_buffers[global_buffer.write_buffer],
        &adc_hw->fifo,
        BUFFER_SIZE,
        false
    );
    
    // Настройка прерываний DMA
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, adc_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    // Начальные настройки
    adc_set_clkdiv(0); // Максимальная частота
    global_buffer.sample_rate = 500000; // 500 kHz
}

void adc_start() {
    if (!adc_running) {
        adc_run(true);
        dma_channel_start(dma_chan);
        adc_running = true;
        global_buffer.running = true;
    }
}

void adc_stop() {
    if (adc_running) {
        adc_run(false);
        dma_channel_abort(dma_chan);
        adc_running = false;
        global_buffer.running = false;
    }
}

/*void buffer_process() {
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (global_buffer.buffer_ready[i]) {
            mutex_enter_blocking(&global_buffer.buffer_mutex);
            
            // Обработка триггера
            if (global_buffer.trigger_enabled) {
                if (!check_trigger(global_buffer.adc_buffers[i])) {
                    global_buffer.buffer_ready[i] = false;
                    mutex_exit(&global_buffer.buffer_mutex);
                    continue;
                }
            }
            
            // Обновление статистики
            buffer_update_stats(global_buffer.adc_buffers[i]);
            
            // Подготовка к отображению
            if (!global_buffer.hold) {
                global_buffer.read_buffer = i;
            }
            
            global_buffer.buffer_ready[i] = false;
            mutex_exit(&global_buffer.buffer_mutex);
        }
    }
}*/


/*bool check_trigger(uint16_t* buffer) {
    for (int i = 1; i < BUFFER_SIZE; i++) {
        if (global_buffer.trigger_edge) { // Rising edge
            if (buffer[i-1] < global_buffer.trigger_level && 
                buffer[i] >= global_buffer.trigger_level) {
                return true;
            }
        } else { // Falling edge
            if (buffer[i-1] > global_buffer.trigger_level && 
                buffer[i] <= global_buffer.trigger_level) {
                return true;
            }
        }
    }
    return false;
}*/


/*
static void buffer_update_stats(uint16_t* buffer) {
    uint16_t min = 4095, max = 0;
    uint32_t sum = 0;
    int crossings = 0;
    bool last_state = buffer[0] > 2048;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] < min) min = buffer[i];
        if (buffer[i] > max) max = buffer[i];
        sum += buffer[i];
        
        // Подсчёт пересечений среднего уровня для частоты
        bool current_state = buffer[i] > (sum / (i+1));
        if (current_state != last_state) {
            crossings++;
            last_state = current_state;
        }
    }
    
    global_buffer.min_value = min;
    global_buffer.max_value = max;
    global_buffer.vpp = max - min;
    
    // Расчёт частоты (если есть хотя бы 2 пересечения)
    if (crossings >= 2) {
        float period_samples = (BUFFER_SIZE * 2.0) / crossings;
        global_buffer.frequency = (uint16_t)(global_buffer.sample_rate / period_samples);
        global_buffer.duty_cycle = 50.0; // Упрощённо, нужен более точный расчёт
    }
}
*/

void adc_processor_task() {
    adc_processor_init();
    adc_start();
    
    while (true) {
        buffer_process();
        tight_loop_contents();
    }
}