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

#define SPI_PORT spi0
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  6
#define PIN_MOSI 7
#define PIN_DC   15
#define PIN_RST  14
#define PIN_LED  20

/* Глобальные переменные модуля */
static ILI9341 tft;  // Объект дисплея
static MenuState menu_state = MENU_NONE;
static bool show_measurements = true;
static absolute_time_t last_redraw;

/* Приватные вспомогательные функции */
static uint16_t constrain(uint16_t val, uint16_t min, uint16_t max) {
    return (val < min) ? min : (val > max) ? max : val;
}

static bool should_redraw(void) {
    return absolute_time_diff_us(last_redraw, get_absolute_time()) > 16666; // 60 FPS
}

/* Публичные функции */

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
    ILI9341_SetRotation(&tft, 3);
    ILI9341_FillScreen(&tft, ILI9341_BLACK);
}

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

void draw_grid(void) {
    ILI9341_FillScreen(&tft, ILI9341_BLACK);
    
    // Вертикальные линии
    for (uint16_t x = 0; x < tft.width; x += 32) {
        ILI9341_DrawLine(&tft, x, 0, x, tft.height, ILI9341_DARKGREY);
    }
    
    // Горизонтальные линии
    for (uint16_t y = 0; y < tft.height; y += 24) {
        ILI9341_DrawLine(&tft, 0, y, tft.width, y, ILI9341_DARKGREY);
    }
    
    // Центральные оси
    ILI9341_DrawLine(&tft, 0, tft.height/2, tft.width, tft.height/2, ILI9341_WHITE);
    ILI9341_DrawLine(&tft, tft.width/2, 0, tft.width/2, tft.height, ILI9341_WHITE);
}

void draw_waveform(void) {
    if (!global_buffer.buffer_ready[global_buffer.read_buffer]) return;
    
    uint16_t* buffer = global_buffer.adc_buffers[global_buffer.read_buffer];
    const float scale = global_buffer.voltage_scale;
    const float offset = global_buffer.voltage_offset;
    const uint16_t mid_y = tft.height / 2;
    
    for (uint16_t i = 1; i < BUFFER_SIZE && i < tft.width; i++) {
        uint16_t y1 = mid_y - (buffer[i-1] - 2048) / 4096.0f * mid_y * scale + offset;
        uint16_t y2 = mid_y - (buffer[i] - 2048) / 4096.0f * mid_y * scale + offset;
        
        y1 = constrain(y1, 0, tft.height-1);
        y2 = constrain(y2, 0, tft.height-1);
        
        ILI9341_DrawLine(&tft, i-1, y1, i, y2, ILI9341_RED);
    }
}

void draw_measurements(void) {
    if (!show_measurements) return;
    
    ILI9341_SetTextColor(&tft, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_SetTextSize(&tft, 1);

    // Напряжения
    ILI9341_SetCursor(&tft, 0, 0);
    ILI9341_Print(&tft, "Vmax: ");
    ILI9341_PrintFloat(&tft, global_buffer.max_value * 3.3f / 4095, 2);

    ILI9341_SetCursor(&tft, 0, 10);
    ILI9341_Print(&tft, "Vmin: ");
    ILI9341_PrintFloat(&tft, global_buffer.min_value * 3.3f / 4095, 2);

    ILI9341_SetCursor(&tft, 0, 20);
    ILI9341_Print(&tft, "Vpp: ");
    ILI9341_PrintFloat(&tft, global_buffer.vpp * 3.3f / 4095, 2);

    // Частота и скважность
    ILI9341_SetCursor(&tft, 100, 0);
    ILI9341_Print(&tft, "Freq: ");
    char freq_buf[16];
    snprintf(freq_buf, sizeof(freq_buf), "%d", global_buffer.frequency);
    ILI9341_Print(&tft, freq_buf);

    ILI9341_SetCursor(&tft, 100, 10);
    ILI9341_Print(&tft, "Duty: ");
    ILI9341_PrintFloat(&tft, global_buffer.duty_cycle, 1);
    ILI9341_Print(&tft, "%");
}

void display_task(void) {
    display_init();
    init_buttons();
    last_redraw = get_absolute_time();
    ILI9341_SetFont(&tft, *font_8x8);
    ILI9341_SetRotation(&tft, 3);
    while (true) {
        /*process_buttons();
        
        if (should_redraw()) {
            draw_grid();
            if (!global_buffer.hold) draw_waveform();
            draw_measurements();
            last_redraw = get_absolute_time();
        }
        
        tight_loop_contents();*/
        ILI9341_SetCursor(&tft, 100, 10);
        ILI9341_Print(&tft, "HELLO WORLD!");
        ILI9341_SetCursor(&tft, 100, 50);
        ILI9341_PrintInteger(&tft, 2025);
        //ILI9341_DrawLine(&tft, 0, tft.height/2, tft.width, tft.height/2, ILI9341_WHITE);

    }
}