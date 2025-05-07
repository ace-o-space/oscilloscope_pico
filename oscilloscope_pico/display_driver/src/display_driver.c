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
//static uint16_t frame_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
//static uint16_t prev_waveform[DISPLAY_WIDTH];

static color8_t* active_buf8 = frame_buf8_1;
static color8_t* draw_buf8 = frame_buf8_2;
static uint16_t prev_waveform[DISPLAY_WIDTH]; // Хранит предыдущие Y-координаты

// static uint16_t screen_buf1[DISPLAY_WIDTH * WAVEFORM_HEIGHT]; // Только область осциллографа
// static uint16_t screen_buf2[DISPLAY_WIDTH * WAVEFORM_HEIGHT];
// static uint16_t* active_buf = screen_buf1;

// Двойной буфер экрана (2 отдельных массива)
//static uint16_t screen_buf1[DISPLAY_WIDTH * DISPLAY_HEIGHT];
//static uint16_t screen_buf2[DISPLAY_WIDTH * DISPLAY_HEIGHT];
//static uint16_t screen_buf2[DISPLAY_WIDTH];
//static uint16_t* active_buf = screen_buf1;  // Указатель на активный буфер

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

    // Очистка буферов
    memset(frame_buf8_1, COLOR8_BLACK, sizeof(frame_buf8_1));
    memset(frame_buf8_2, COLOR8_BLACK, sizeof(frame_buf8_2));
}

void swap_buffers() {
    // Меняем буферы местами
    color8_t* temp = active_buf8;
    active_buf8 = draw_buf8;
    draw_buf8 = temp;
    
    // Конвертируем и выводим
    ILI9341_DrawBuffer8to16(&tft, active_buf8);
}

/*
void swap_buffers(void) {
    // Переключаем активный буфер
    active_buf = (active_buf == screen_buf1) ? screen_buf2 : screen_buf1;
    
    // Отправляем только область осциллографа
    ILI9341_DrawBufferDMA(
        &tft,
        0, 
        WAVEFORM_TOP, // Начинаем с Y=30
        DISPLAY_WIDTH,
        WAVEFORM_HEIGHT,  // Только 290 строк (320-30)
        active_buf
    );
}
*/

void draw_grid(void) {
    for (uint16_t y = 30; y < DISPLAY_WIDTH; y = y + 50) {
        for (uint16_t x = 10; x < DISPLAY_HEIGHT; x = x + 5) { 
            ILI9341_DrawPixel(&tft, x, y, COLOR8_GRAY);
        }
    }
    for (uint16_t x = 0; x < DISPLAY_HEIGHT; x = x + 64) {
        for (uint16_t y = 40; y < DISPLAY_WIDTH; y = y + 10) { 
            ILI9341_DrawPixel(&tft, x, y, COLOR8_GRAY);
        }
    }
}

void draw_waveform(uint16_t* adc_data) {
    // 1. Очищаем только область осциллографа
    memset(draw_buf8 + WAVEFORM_TOP * DISPLAY_WIDTH, 
           COLOR8_BLACK, 
           DISPLAY_WIDTH * WAVEFORM_HEIGHT);
    
    // 2. Рисуем сигнал
    for (int x = 0; x < DISPLAY_WIDTH && x < BUFFER_SIZE; x++) {
        int y = WAVEFORM_TOP + ((4095 - adc_data[x]) * WAVEFORM_HEIGHT / 4096);
        y = (y < WAVEFORM_TOP) ? WAVEFORM_TOP : 
            (y >= DISPLAY_HEIGHT) ? DISPLAY_HEIGHT-1 : y;
        
        // Пиксель сигнала
        draw_buf8[y * DISPLAY_WIDTH + x] = COLOR8_RED;
        
        // Подсветка (опционально)
        //if (x % 2 == 0) {
        //    draw_buf8[(y+1) * DISPLAY_WIDTH + x] = COLOR8_RED;
        //}
    }
    
    draw_grid();
    swap_buffers();
}

/*
void draw_waveform(uint16_t* adc_buffer) {
    // Определяем буфер для рисования
    uint16_t* draw_buf = (active_buf == screen_buf1) ? screen_buf2 : screen_buf1;
    
    // Константы для области отрисовки
    const int start_y = 30; // Начинаем стирать с этой координаты Y
    const int draw_height = DISPLAY_HEIGHT - start_y;
    
    // Частичное стирание фона (только нужная область)
    memset(draw_buf + (start_y * DISPLAY_WIDTH), 
           0, 
           sizeof(uint16_t) * DISPLAY_WIDTH * draw_height);
    
    // Рисуем сетку (если нужно)
    draw_grid();
    
    // Рисуем сигнал (только в активной области)
    for (int x = 0; x < DISPLAY_WIDTH && x < BUFFER_SIZE; x++) {
        // Масштабируем значение ADC (0-4095) в координаты экрана
        int y = start_y + ((4095 - adc_buffer[x]) * draw_height / 4096);
        
        // Ограничиваем координаты
        y = (y < start_y) ? start_y : (y >= DISPLAY_HEIGHT) ? DISPLAY_HEIGHT-1 : y;
        
        // Рисуем пиксель
        draw_buf[y * DISPLAY_WIDTH + x] = ILI9341_RED;
        
        // Стираем предыдущее положение сигнала (если нужно)
        if (prev_waveform[x] != y) {
            draw_buf[prev_waveform[x] * DISPLAY_WIDTH + x] = ILI9341_BLACK;
        }
    }
    
    // Обновляем дисплей
    swap_buffers();
}
*/
/*
void draw_waveform(uint16_t* adc_buffer) {
    uint16_t* draw_buf = (active_buf == screen_buf1) ? screen_buf2 : screen_buf1;
    
    // 1. Стираем предыдущий сигнал
    for (int x = 0; x < DISPLAY_WIDTH && x < BUFFER_SIZE; x++) {
        if (prev_waveform[x] > 0) { // 0 - значение по умолчанию
            draw_buf[(prev_waveform[x] - WAVEFORM_TOP) * DISPLAY_WIDTH + x] = COLOR_BLACK;
        }
    }
    
    // 2. Рисуем новый сигнал
    for (int x = 0; x < DISPLAY_WIDTH && x < BUFFER_SIZE; x++) {
        int y = WAVEFORM_TOP + ((4095 - adc_buffer[x]) * WAVEFORM_HEIGHT / 4096);
        y = (y < WAVEFORM_TOP) ? WAVEFORM_TOP : 
            (y >= DISPLAY_HEIGHT) ? DISPLAY_HEIGHT-1 : y;
        
        // Сохраняем новую позицию (относительно области осциллографа)
        prev_waveform[x] = y;
        
        // Рисуем в буфере (учитываем смещение Y)
        draw_buf[(y - WAVEFORM_TOP) * DISPLAY_WIDTH + x] = COLOR_BLUE;
    }
    
    swap_buffers();
}
*/
/*
void draw_waveform(uint16_t* buffer) {
    const uint16_t mid_y = 120;
    uint16_t* current_buf = screen_buf[active_buf ^ 1];
    
    // Копируем фон
    memcpy(current_buf, screen_buf[active_buf], sizeof(screen_buf[0]));
    
    // Рисуем новый сигнал
    for (int i = 0; i < BUFFER_SIZE && i < 320; i++) {
        uint16_t y = mid_y - (buffer[i] - 2048) * mid_y / 4096;
        y = (y < 30) ? 30 : (y > 230) ? 230 : y;
        
        current_buf[y * 320 + i] = ILI9341_RED;
        current_buf[(y+1) * 320 + i] = ILI9341_RED;
        
        // Стираем предыдущий сигнал
        if (prev_waveform[i] != y) {
            current_buf[prev_waveform[i] * 320 + i] = ILI9341_BLACK;
            current_buf[(prev_waveform[i]+1) * 320 + i] = ILI9341_BLACK;
        }
    }
    
    memcpy(prev_waveform, buffer, sizeof(prev_waveform));
    swap_buffers();
}
*/
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
    measurements[0] = global_buffer.max_value * 3.3f / 4095;
    measurements[1] = global_buffer.min_value * 3.3f / 4095;
    measurements[2] = global_buffer.vpp * 3.3f / 4095;
    measurements[3] = global_buffer.frequency;
    measurements[4] = global_buffer.duty_cycle;
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

    //if (!show_measurements) return;
    
    ILI9341_SetTextColor(&tft, COLOR8_WHITE, COLOR8_BLACK);
    ILI9341_SetTextSize(&tft, 1);

    // Напряжения
    ILI9341_SetCursor(&tft, 0, 0);
    ILI9341_Print(&tft, "Vmax,V: ");
    ILI9341_PrintFloat(&tft, measurements[0], 2);
    
    ILI9341_SetCursor(&tft, 0, 10);
    ILI9341_Print(&tft, "Vmin,V: ");
    ILI9341_PrintFloat(&tft, measurements[1], 2);

    ILI9341_SetCursor(&tft, 0, 20);
    ILI9341_Print(&tft, "Vpp,V: ");
    ILI9341_PrintFloat(&tft, measurements[2], 2);

    // Частота и скважность
    ILI9341_SetCursor(&tft, 150, 0);
    ILI9341_Print(&tft, "Freq,Hz: ");
    char freq_buf[16];
    snprintf(freq_buf, sizeof(freq_buf), "%d", measurements[3]);
    ILI9341_Print(&tft, freq_buf);

    ILI9341_SetCursor(&tft, 150, 10);
    ILI9341_Print(&tft, "Duty,%: ");
    ILI9341_PrintFloat(&tft, measurements[4], 1);
}

void erase_measurements(float* measurements){
    //if (!show_measurements) return;
    
    ILI9341_SetTextColor(&tft, COLOR8_BLACK, COLOR8_BLACK);
    ILI9341_SetTextSize(&tft, 1);

    // Напряжения
    ILI9341_SetCursor(&tft, 0, 0);
    ILI9341_Print(&tft, "Vmax, V: ");
    ILI9341_PrintFloat(&tft, measurements[0], 2);
    
    ILI9341_SetCursor(&tft, 0, 10);
    ILI9341_Print(&tft, "Vmin, V: ");
    ILI9341_PrintFloat(&tft, measurements[1], 2);

    ILI9341_SetCursor(&tft, 0, 20);
    ILI9341_Print(&tft, "Vpp, V: ");
    ILI9341_PrintFloat(&tft, measurements[2], 2);

    // Частота и скважность
    ILI9341_SetCursor(&tft, 100, 0);
    ILI9341_Print(&tft, "Freq, Hz: ");
    char freq_buf[16];
    snprintf(freq_buf, sizeof(freq_buf), "%d", measurements[3]);
    ILI9341_Print(&tft, freq_buf);

    ILI9341_SetCursor(&tft, 100, 10);
    ILI9341_Print(&tft, "Duty, %: ");
    ILI9341_PrintFloat(&tft, measurements[4], 1);
}

void __not_in_flash_func(core1_display_task)(void) {
    // Инициализация дисплея
    display_init();
    init_buttons();

    while (true) {
        float measurements[5];
        float voltage_constants[2];
        uint16_t *current_adc_buffer;

        get_values_for_draw(current_adc_buffer, measurements, voltage_constants);
        //process_buttons();

        draw_grid();
        draw_measurements(measurements);

        if (multicore_fifo_rvalid()) {
            uint8_t buf_idx = multicore_fifo_pop_blocking();
            
            mutex_enter_blocking(&global_buffer.buffer_mutex);
            if (global_buffer.buffer_ready[buf_idx]) {
                draw_waveform(global_buffer.adc_buffers[buf_idx]);
                global_buffer.buffer_ready[buf_idx] = false;
            }
            mutex_exit(&global_buffer.buffer_mutex);
        }
        
        // Обработка UI (30 FPS)
        static absolute_time_t last_ui = 0;
        if (absolute_time_diff_us(last_ui, get_absolute_time()) > 33333) {
            last_ui = get_absolute_time();
            process_buttons();
            //erase_measurements(measurements);
        }
    }
}