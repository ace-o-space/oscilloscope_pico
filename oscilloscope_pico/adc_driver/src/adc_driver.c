#include "adc_driver/adc_driver.h"
#include "global_buffer/global_buffer.h"
#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/multicore.h>

static int dma_chan;
static volatile bool adc_running = false;
extern void buffer_update_stats(uint16_t* buffer);
extern bool check_trigger(uint16_t* buffer);
extern void buffer_process();


void adc_dma_handler() {
    if (dma_channel_get_irq0_status(dma_chan)) {
        dma_channel_acknowledge_irq0(dma_chan);
        
        mutex_enter_blocking(&global_buffer.buffer_mutex);
        uint8_t filled_buf = global_buffer.write_buffer;
        
        // Всегда обновляем display_buffer в live-режиме
        if (global_buffer.live_update && !global_buffer.hold) {
            global_buffer.display_buffer = filled_buf;
        }
        
        // Переключаем буфер
        global_buffer.write_buffer = (filled_buf + 1) % NUM_BUFFERS;
        global_buffer.buffer_ready[filled_buf] = true;
        
        multicore_fifo_push_blocking(filled_buf);
        mutex_exit(&global_buffer.buffer_mutex);
        
        dma_channel_set_write_addr(dma_chan, 
            global_buffer.adc_buffers[global_buffer.write_buffer], 
            true);
    }
}

void adc_processor_init() {
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    adc_fifo_setup(true, true, 1, false, false);
    
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
    
    dma_channel_configure(dma_chan, &cfg,
        global_buffer.adc_buffers[0],
        &adc_hw->fifo,
        BUFFER_SIZE,
        false
    );
    
    irq_set_exclusive_handler(DMA_IRQ_0, adc_dma_handler);
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_enabled(DMA_IRQ_0, true);
    
    adc_set_clkdiv(0);
    global_buffer.sample_rate = 500000; // 500 kHz
}

void adc_start() {
    if (!global_buffer.running) {
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

void core0_adc_task() {
    adc_processor_init();
    adc_start();
    
    while (true) {
        __wfe();
        //buffer_process();
        //tight_loop_contents();
    }
}
