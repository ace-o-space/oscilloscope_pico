#include <stdlib.h>
#include "pico_ili9341/pico_ili9341.h"
#include "hardware/dma.h"
#include "hardware/spi.h"

extern void write_data(ILI9341 *disp, const uint8_t *data, size_t len);
extern void write_command(ILI9341 *disp, uint8_t cmd);
extern void write_command_data(ILI9341 *disp, uint8_t cmd, const uint8_t *data, uint8_t len);

// Конвертация RGB888 в RGB565
static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Настройка DMA (вызывается один раз при инициализации)
static int dma_channel = -1;
static dma_channel_config dma_config;
static int dma_chan;
void ILI9341_SetupDMA(ILI9341 *disp) {
    if (!disp->dma) return;
    
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(disp->spi, true));
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
}

// Оптимизированная функция для рисования буфера с DMA
void ILI9341_DrawBufferDMA(
    ILI9341 *disp,
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t *buffer  // Принимает указатель на uint16_t
) {
    // Установка области вывода
    uint8_t caset_data[4] = {
        x >> 8, x & 0xFF,
        (x + w - 1) >> 8, (x + w - 1) & 0xFF
    };
    write_command_data(disp, ILI9341_CASET, caset_data, 4);
    
    uint8_t paset_data[4] = {
        y >> 8, y & 0xFF,
        (y + h - 1) >> 8, (y + h - 1) & 0xFF
    };
    write_command_data(disp, ILI9341_PASET, paset_data, 4);
    
    // Запуск DMA передачи
    write_command(disp, ILI9341_RAMWR);
    gpio_put(disp->dc_pin, 1);
    gpio_put(disp->cs_pin, 0);
    
    if (disp->dma && dma_channel >= 0) {
        dma_channel_set_read_addr(dma_channel, buffer, false);
        dma_channel_set_trans_count(dma_channel, w * h, true);
        dma_channel_wait_for_finish_blocking(dma_channel);
    } else {
        spi_write_blocking(disp->spi, (uint8_t*)buffer, w * h * 2);
    }
    
    gpio_put(disp->cs_pin, 1);
}

// Оптимизированная заливка прямоугольника
void ILI9341_FillRectDMA(ILI9341 *disp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint8_t color_buf[2] = {color >> 8, color & 0xFF};
    uint8_t dma_buf[w * 2];
    
    for (int i = 0; i < w; i++) {
        dma_buf[i*2] = color_buf[0];
        dma_buf[i*2+1] = color_buf[1];
    }
    
    ILI9341_SetAddressWindow(disp, x, y, x+w-1, y+h-1);
    
    gpio_put(disp->cs_pin, 0);
    gpio_put(disp->dc_pin, 1);
    
    for (int line = 0; line < h; line++) {
        dma_channel_set_read_addr(dma_chan, dma_buf, true);
        dma_channel_set_trans_count(dma_chan, w*2, true);
        dma_channel_wait_for_finish_blocking(dma_chan);
    }
    
    gpio_put(disp->cs_pin, 1);
}

// Рисование линии (оптимизированная версия)
void ILI9341_DrawLine(ILI9341 *disp, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    
    uint8_t color_bytes[2] = {color >> 8, color & 0xFF};
    
    while (1) {
        // Установка позиции пикселя
        uint8_t caset_data[4] = {x0 >> 8, x0 & 0xFF, x0 >> 8, x0 & 0xFF};
        write_command_data(disp, ILI9341_CASET, caset_data, 4);
        
        uint8_t paset_data[4] = {y0 >> 8, y0 & 0xFF, y0 >> 8, y0 & 0xFF};
        write_command_data(disp, ILI9341_PASET, paset_data, 4);
        
        // Запись пикселя
        write_command(disp, ILI9341_RAMWR);
        write_data(disp, color_bytes, 2);
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ILI9341_DrawPixel(ILI9341 *disp, uint16_t x, uint16_t y, color8_t color) {
    if (x >= disp->width || y >= disp->height) return;
    frame_buffer[y * disp->width + x] = color;
}

// Оптимизированное рисование прямоугольника
void ILI9341_DrawRect(ILI9341 *disp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    ILI9341_DrawLine(disp, x, y, x + w - 1, y, color);         // Верхняя линия
    ILI9341_DrawLine(disp, x, y + h - 1, x + w - 1, y + h - 1, color); // Нижняя линия
    ILI9341_DrawLine(disp, x, y, x, y + h - 1, color);         // Левая линия
    ILI9341_DrawLine(disp, x + w - 1, y, x + w - 1, y + h - 1, color); // Правая линия
}
