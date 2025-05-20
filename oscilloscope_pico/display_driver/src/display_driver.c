#include "display_driver/display_driver.h"
#include "pico_ili9341/pico_ili9341.h"
#include "global_buffer/global_buffer.h"
#include "pico_ili9341/font_5x7.h"
#include "pico_ili9341/font_8x8.h"
#include "pico_ili9341/font_12x16.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pico/time.h"
#include <stdio.h> 
#include <string.h>
#include <pico/multicore.h>

#define SPI_PORT spi0
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  6
#define PIN_MOSI 7
#define PIN_DC   15
#define PIN_RST  14
#define PIN_LED  20

/* Глобальные переменные модуля */
static ILI9341 tft;
static MenuState menu_state = MENU_NONE;
static bool show_measurements = true;
absolute_time_t last_redraw;
static uint16_t prev_waveform[DISPLAY_WIDTH]; // Хранит предыдущие Y-координаты

// 8-битные буферы только для области осциллографа (WAVEFORM_HEIGHT = 210, WAVEFORM_WIDTH = 320)
color8_t waveform_buf1[WAVEFORM_WIDTH * WAVEFORM_HEIGHT];
color8_t waveform_buf2[WAVEFORM_WIDTH * WAVEFORM_HEIGHT];
color8_t* active_wave_buf = waveform_buf1;
color8_t* draw_wave_buf = waveform_buf2;

extern void ILI9341_FillRectDMA(ILI9341 *disp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
extern bool adc_get_buffer(uint16_t** buffer);

void display_init(void) {

    // Конфигурация SPI дисплея
    ILI9341Config config = {
        .spi = SPI_PORT,
        .cs_pin = PIN_CS,
        .dc_pin = PIN_DC,
        .rst_pin = PIN_RST,
        .sck_pin = PIN_SCK,   
        .mosi_pin = PIN_MOSI,  
        .miso_pin = PIN_MISO, 
        .baudrate = 60 * 1000 * 1000,
        .dma = true
    };

    // Инициализация дисплея
    ILI9341_Init(&tft, &config);
    ILI9341_SetFont(&tft, *font_8x8);
    ILI9341_SetRotation(&tft, 3);
    ILI9341_FillScreen(&tft, COLOR8_BLACK);
    //ILI9341_SetPalette(&tft, 0x28);

    // Очистка буферов
    memset(waveform_buf1, COLOR8_BLACK, sizeof(waveform_buf1));
    memset(waveform_buf2, COLOR8_BLACK, sizeof(waveform_buf2));
}

void swap_buffers() {
    // Меняем буферы местами
    color8_t* temp = active_wave_buf;
    active_wave_buf = draw_wave_buf;
    draw_wave_buf = temp;
    
    // Конвертируем и выводим
    ILI9341_DrawBuffer8to16(&tft, active_wave_buf);
}

void swap_wave_buffers() {
    // Меняем буферы волны местами
    color8_t* temp = active_wave_buf;
    active_wave_buf = draw_wave_buf;
    draw_wave_buf = temp;
    
    ILI9341_DrawBuffer8to16(&tft, active_wave_buf);

}


void draw_grid(void) {
    //сетка
    for (int y = 0; y < WAVEFORM_HEIGHT; y += 50) {
        for (int x = 10; x < WAVEFORM_WIDTH; x += 5) {
            draw_wave_buf[y * WAVEFORM_WIDTH + x] = COLOR8_GRAY;
        }
    }
}

void draw_waveform(uint16_t* adc_data) {
    // Очищаем буфер рисования (только область осциллографа)
    memset(draw_wave_buf, COLOR8_BLACK, sizeof(waveform_buf1));
    
    draw_grid();

    // Сигнал
    for (int x = 0; x < WAVEFORM_WIDTH; x++) {
        int y = ((4095 - adc_data[x]) * WAVEFORM_HEIGHT / 4096);
        draw_wave_buf[y * WAVEFORM_WIDTH + x] = COLOR8_RED;
    }
}

/* Публичные функции */

void init_buttons(void) {
    gpio_init(BUTTON_HOLD);
    gpio_init(BUTTON_PLUS);
    gpio_init(BUTTON_MINUS);
    gpio_init(BUTTON_SET);
    
    gpio_set_dir(BUTTON_HOLD, GPIO_IN);
    gpio_set_dir(BUTTON_PLUS, GPIO_IN);
    gpio_set_dir(BUTTON_MINUS, GPIO_IN);
    gpio_set_dir(BUTTON_SET, GPIO_IN);
    
    gpio_pull_up(BUTTON_HOLD);
    gpio_pull_up(BUTTON_PLUS);
    gpio_pull_up(BUTTON_MINUS);
    gpio_pull_up(BUTTON_SET);
}

void process_buttons(void) {
    static absolute_time_t last_press = 0;
    if (absolute_time_diff_us(last_press, get_absolute_time()) < 20000) return;
    
    if (!gpio_get(BUTTON_SET)) {
        menu_state = (menu_state + 1) % (MENU_TRIGGER + 1);
        last_press = get_absolute_time();
    }
    
    if (!gpio_get(BUTTON_HOLD)) {
        global_buffer.hold = !global_buffer.hold;
        last_press = get_absolute_time();
    }
    
    float* adjust_value = NULL;
    switch (menu_state) {
        case MENU_TIME_SCALE: adjust_value = &global_buffer.time_scale; break;
        case MENU_VOLT_SCALE: adjust_value = &global_buffer.voltage_scale; break;
        case MENU_TRIGGER: adjust_value = (float*)&global_buffer.trigger_level; break;
        default: break;
    }
    
    if (adjust_value) {
        if (!gpio_get(BUTTON_PLUS)) *adjust_value *= 1.2f;
        if (!gpio_get(BUTTON_MINUS)) *adjust_value /= 1.2f;
        last_press = get_absolute_time();
    }
}

static void get_measurements(float *measurements){
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    measurements[0] = global_buffer.max_value;
    measurements[1]= global_buffer.min_value;
    measurements[2] = global_buffer.vpp;
    measurements[3] = global_buffer.frequency;
    measurements[4] = global_buffer.duty_cycle;
    mutex_exit(&global_buffer.buffer_mutex);
}

static void get_current_adc_buffer(uint16_t *buffer){
    buffer = global_buffer.adc_buffers[global_buffer.read_buffer];
}

static void get_voltage_constants(float *voltage_constants){
    voltage_constants[0] = global_buffer.voltage_scale;
    voltage_constants[1] = global_buffer.voltage_offset;
}

void get_values_for_draw(uint16_t *current_adc_buffer, float *measurements, float *voltage_constants){
    get_current_adc_buffer(current_adc_buffer);
    get_measurements(measurements);
    get_voltage_constants(voltage_constants);
}

void draw_measurements(float *measurements) {
    // Безопасное получение текущих измерений
    //mutex_enter_blocking(&global_buffer.buffer_mutex);
    float v_max = measurements[0] * 3.3f / 4095;
    float v_min = measurements[1] * 3.3f / 4095;
    float v_pp = measurements[2] * 3.3f / 4095;
    float freq = measurements[3];
    float duty = measurements[4];
    //mutex_exit(&global_buffer.buffer_mutex);
    
    // Отрисовка
    ILI9341_SetTextColor(&tft, COLOR8_WHITE, COLOR8_BLACK);
    ILI9341_SetTextSize(&tft, 1);
    
    ILI9341_SetCursor(&tft, 0, 210);
    ILI9341_Print(&tft, "Vmax,V: ");
    ILI9341_PrintFloat(&tft, v_max, 2);
    
    ILI9341_SetCursor(&tft, 0, 220);
    ILI9341_Print(&tft, "Vmin,V: ");
    ILI9341_PrintFloat(&tft, v_min, 2);

    ILI9341_SetCursor(&tft, 0, 230);
    ILI9341_Print(&tft, "Vpp,V: ");
    ILI9341_PrintFloat(&tft, v_pp, 2);

    // Частота и скважность
    ILI9341_SetCursor(&tft, 150, 210);
    ILI9341_Print(&tft, "Freq,Hz: ");
    char freq_buf[16];
    snprintf(freq_buf, sizeof(freq_buf), "%d", freq);
    ILI9341_Print(&tft, freq_buf);

    ILI9341_SetCursor(&tft, 150, 220);
    ILI9341_Print(&tft, "Duty,%: ");
    ILI9341_PrintFloat(&tft, duty, 1);
}

void render_frame() {
    // 1. Получаем текущий буфер для отображения
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    uint8_t buf_to_display = global_buffer.display_buffer;
    bool live_mode = global_buffer.live_update;
    mutex_exit(&global_buffer.buffer_mutex);
    
    // 2. Отрисовка измерений (всегда актуальные)
    float measurements[5];
    get_measurements(measurements); // Читает из global_buffer
    
    draw_measurements(measurements);
    
    // 3. Отрисовка волны
    if (live_mode || !global_buffer.hold) {
        draw_waveform(global_buffer.adc_buffers[buf_to_display]);
        swap_wave_buffers();
    }
}

void set_hold(bool enable) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.hold = enable;
    
    // При выходе из hold переключаем на последний буфер
    if (!enable) {
        global_buffer.display_buffer = global_buffer.write_buffer;
    }
    mutex_exit(&global_buffer.buffer_mutex);
}

void set_live_update(bool enable) {
    mutex_enter_blocking(&global_buffer.buffer_mutex);
    global_buffer.live_update = enable;
    mutex_exit(&global_buffer.buffer_mutex);
}

void core1_display_task() {
    display_init();
    init_buttons();
    
    const uint32_t target_fps = 120;
    const uint32_t frame_delay_us = 1000000 / target_fps;
    
    while (1) {
        absolute_time_t frame_start = get_absolute_time();
        
        // Обработка новых данных
        if (multicore_fifo_rvalid()) {
            uint8_t buf_idx = multicore_fifo_pop_blocking();
            buffer_process(buf_idx);
        }
        
        // Рендеринг
        render_frame();
        
        // Обработка UI
        process_buttons();
        
        // Поддержание FPS
        busy_wait_until(frame_start + frame_delay_us);
    }
}
