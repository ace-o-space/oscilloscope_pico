#include "pico_ili9341/pico_ili9341.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "pico/time.h"

// Приватные функции
void write_command(ILI9341 *disp, uint8_t cmd) {
    gpio_put(disp->dc_pin, 0);
    gpio_put(disp->cs_pin, 0);
    spi_write_blocking(disp->spi, &cmd, 1);
    gpio_put(disp->cs_pin, 1);
}

void write_data(ILI9341 *disp, const uint8_t *data, size_t len) {
    gpio_put(disp->dc_pin, 1);
    gpio_put(disp->cs_pin, 0);
    spi_write_blocking(disp->spi, data, len);
    gpio_put(disp->cs_pin, 1);
}

void write_command_data(ILI9341 *disp, uint8_t cmd, const uint8_t *data, uint8_t len) {
    write_command(disp, cmd);
    write_data(disp, data, len);
}

// Полная последовательность инициализации ILI9341
void ILI9341_Init(ILI9341 *disp, const ILI9341Config *config) {
    // Инициализация структуры дисплея
    disp->spi = config->spi;
    disp->cs_pin = config->cs_pin;
    disp->dc_pin = config->dc_pin;
    disp->rst_pin = config->rst_pin;
    disp->baudrate = config->baudrate;
    disp->dma = config->dma;
    disp->width = ILI9341_WIDTH;
    disp->height = ILI9341_HEIGHT;
    disp->rotation = 0;

    // Инициализация GPIO
    gpio_init(disp->cs_pin);
    gpio_init(disp->dc_pin);
    gpio_init(disp->rst_pin);
    
    gpio_set_dir(disp->cs_pin, GPIO_OUT);
    gpio_set_dir(disp->dc_pin, GPIO_OUT);
    gpio_set_dir(disp->rst_pin, GPIO_OUT);
    
    // Начальное состояние пинов
    gpio_put(disp->cs_pin, 1);
    gpio_put(disp->dc_pin, 0);
    
    // Процедура сброса
    gpio_put(disp->rst_pin, 1);
    sleep_ms(5);
    gpio_put(disp->rst_pin, 0);
    sleep_ms(20);
    gpio_put(disp->rst_pin, 1);
    sleep_ms(150);
    
    // Инициализация SPI
    spi_init(disp->spi, disp->baudrate);
    gpio_set_function(config->sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(config->mosi_pin, GPIO_FUNC_SPI);
    if (config->miso_pin != -1) {
        gpio_set_function(config->miso_pin, GPIO_FUNC_SPI);
    }
    
    // Полная последовательность команд инициализации
    write_command(disp, 0xEF);
    uint8_t data3[] = {0x03, 0x80, 0x02};
    write_data(disp, data3, 3);
    
    write_command(disp, 0xCF);
    uint8_t data3a[] = {0x00, 0xC1, 0x30};
    write_data(disp, data3a, 3);
    
    write_command(disp, 0xED);
    uint8_t data4[] = {0x64, 0x03, 0x12, 0x81};
    write_data(disp, data4, 4);
    
    write_command(disp, 0xE8);
    uint8_t data3b[] = {0x85, 0x00, 0x78};
    write_data(disp, data3b, 3);
    
    write_command(disp, 0xCB);
    uint8_t data5[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    write_data(disp, data5, 5);
    
    write_command(disp, 0xF7);
    uint8_t data1a[] = {0x20};
    write_data(disp, data1a, 1);
    
    write_command(disp, 0xEA);
    uint8_t data2a[] = {0x00, 0x00};
    write_data(disp, data2a, 2);
    
    // Power control
    write_command(disp, 0xC0);    // Power control
    uint8_t data1b[] = {0x23};    // VRH[5:0]
    write_data(disp, data1b, 1);
    
    write_command(disp, 0xC1);    // Power control
    uint8_t data1c[] = {0x10};    // SAP[2:0];BT[3:0]
    write_data(disp, data1c, 1);
    
    // VCM control
    write_command(disp, 0xC5);    // VCM control
    uint8_t data2b[] = {0x3e, 0x28}; // Contrast
    write_data(disp, data2b, 2);
    
    write_command(disp, 0xC7);    // VCM control2
    uint8_t data1d[] = {0x86};    // --
    write_data(disp, data1d, 1);
    
    // Memory Access Control
    write_command(disp, 0x36);    // Memory Access Control
    uint8_t data1e[] = {0x48};    // MX, BGR
    write_data(disp, data1e, 1);
    
    // Pixel format
    write_command(disp, 0x3A);    // Pixel Format
    uint8_t data1f[] = {0x55};    // 16 bit/pixel
    write_data(disp, data1f, 1);
    
    // Frame rate
    write_command(disp, 0xB1);    // Frame Rate
    uint8_t data2c[] = {0x00, 0x18}; // 70Hz
    write_data(disp, data2c, 2);
    
    // Display Function Control
    write_command(disp, 0xB6);    // Display Function Control
    uint8_t data4a[] = {0x08, 0x82, 0x27};
    write_data(disp, data4a, 3);
    
    // Gamma Function Disable
    write_command(disp, 0xF2);    // Gamma Function Disable
    uint8_t data1g[] = {0x00};
    write_data(disp, data1g, 1);
    
    // Gamma curve
    write_command(disp, 0x26);    // Gamma curve
    uint8_t data1h[] = {0x01};    // Gamma set 1
    write_data(disp, data1h, 1);
    
    // Positive Gamma Correction
    write_command(disp, 0xE0);
    uint8_t gamma_pos[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 
                          0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    write_data(disp, gamma_pos, 15);
    
    // Negative Gamma Correction
    write_command(disp, 0xE1);
    uint8_t gamma_neg[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 
                          0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    write_data(disp, gamma_neg, 15);
    
    // Exit Sleep
    write_command(disp, 0x11);    // Exit Sleep
    sleep_ms(120);
    
    // Display ON
    write_command(disp, 0x29);    // Display ON

}

void ILI9341_SetRotation(ILI9341 *disp, uint8_t rotation) {
    write_command(disp, ILI9341_MADCTL);
    
    uint8_t madctl = 0;
    disp->rotation = rotation % 4;
    
    switch (disp->rotation) {
        case 0:
            madctl = 0x48;
            disp->width = ILI9341_WIDTH;
            disp->height = ILI9341_HEIGHT;
            break;
        case 1:
            madctl = 0x28;
            disp->width = ILI9341_HEIGHT;
            disp->height = ILI9341_WIDTH;
            break;
        case 2:
            madctl = 0x88;
            disp->width = ILI9341_WIDTH;
            disp->height = ILI9341_HEIGHT;
            break;
        case 3:
            madctl = 0xE8;
            disp->width = ILI9341_HEIGHT;
            disp->height = ILI9341_WIDTH;
            break;
    }
    
    write_data(disp, &madctl, 1);
}

void ILI9341_FillScreen(ILI9341 *disp, uint16_t color) {
    uint8_t color_bytes[2] = {color >> 8, color & 0xFF};
    
    // Установка области заполнения
    uint8_t caset_data[4] = {0x00, 0x00, disp->width >> 8, disp->width & 0xFF};
    write_command_data(disp, ILI9341_CASET, caset_data, 4);
    
    uint8_t paset_data[4] = {0x00, 0x00, disp->height >> 8, disp->height & 0xFF};
    write_command_data(disp, ILI9341_PASET, paset_data, 4);
    
    // Заполнение цветом
    write_command(disp, ILI9341_RAMWR);
    gpio_put(disp->dc_pin, 1);
    gpio_put(disp->cs_pin, 0);
    
    if (disp->dma) {
        // DMA-версия для быстрой заливки
        uint16_t buffer[64];
        for (int i = 0; i < 64; i++) buffer[i] = color;
        
        for (int i = 0; i < (disp->width * disp->height) / 64; i++) {
            spi_write_blocking(disp->spi, (uint8_t*)buffer, 128);
        }
    } else {
        // Программная версия
        for (int i = 0; i < disp->width * disp->height; i++) {
            spi_write_blocking(disp->spi, color_bytes, 2);
        }
    }
    
    gpio_put(disp->cs_pin, 1);
}
