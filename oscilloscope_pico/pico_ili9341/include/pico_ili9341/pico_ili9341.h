#pragma once
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320

//Цвета в формате RGB565
//#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
//#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
//#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
//#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
//#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
//#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
//#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
//#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
//#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
//#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
//#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
//#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
//#define ILI9341_RED         0xF800      /* 255,   0,   0 */
//#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
//#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
//#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
//#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
//#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
//#define ILI9341_PINK        0xF81F      /* 255, 192, 203 */

#define ILI9341_SWRESET   0x01
#define ILI9341_SLPIN     0x10
#define ILI9341_SLPOUT    0x11
#define ILI9341_INVOFF    0x20
#define ILI9341_INVON     0x21
#define ILI9341_DISPOFF   0x28
#define ILI9341_DISPON    0x29
#define ILI9341_CASET     0x2A
#define ILI9341_PASET     0x2B
#define ILI9341_RAMWR     0x2C
#define ILI9341_MADCTL    0x36

typedef uint8_t color8_t;

// Стандартные цвета (индексы)
enum {
    COLOR8_BLACK = 0,
    COLOR8_RED,
    COLOR8_GREEN,
    COLOR8_BLUE,
    COLOR8_WHITE,
    COLOR8_GRAY,
    COLOR8_YELLOW,
    COLOR8_CYAN,
    COLOR8_MAGENTA,
    // Добавьте свои цвета
    COLOR8_COUNT
};

// Таблица преобразования в RGB565 (хранится во Flash)
extern const uint16_t color_palette[COLOR8_COUNT];

typedef struct {
    spi_inst_t *spi;
    uint cs_pin;
    uint dc_pin;
    uint rst_pin;
    uint sck_pin;   // SCK пин SPI
    uint mosi_pin;  // MOSI пин SPI
    int miso_pin;   // MISO пин SPI (может быть -1 если не используется)
    uint baudrate;
    bool dma;
} ILI9341Config;

typedef struct {
    spi_inst_t *spi;
    uint cs_pin;
    uint dc_pin;
    uint rst_pin;
    uint baudrate;
    bool dma;
    uint16_t width;
    uint16_t height;
    uint8_t rotation;
} ILI9341;

// 8-битный буфер кадра
static uint8_t frame_buffer[ILI9341_WIDTH * ILI9341_HEIGHT];

// Инициализация
void ILI9341_Init(ILI9341 *disp, const ILI9341Config *config);
void ILI9341_SetRotation(ILI9341 *disp, uint8_t rotation);

// Основные функции
//void ILI9341_SetRotation(ILI9341 *disp, display_rotation_t rotation);
void ILI9341_FillScreen(ILI9341 *disp, uint16_t color);
void ILI9341_FillScreen8(ILI9341 *disp, color8_t color_index);\
void ILI9341_DrawPixel(ILI9341 *disp, uint16_t x, uint16_t y, color8_t color);
void ILI9341_DrawLine(ILI9341 *disp, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ILI9341_DrawRect(ILI9341 *disp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_FillRect(ILI9341 *disp, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_SetPalette(ILI9341 *disp, uint8_t mac_data);

// Текст
void ILI9341_SetFont(ILI9341 *disp, const uint8_t *font);
void ILI9341_SetTextColor(ILI9341 *disp, uint16_t fg, uint16_t bg);
void ILI9341_SetTextSize(ILI9341 *disp, uint8_t size);
void ILI9341_SetCursor(ILI9341 *disp, uint16_t x, uint16_t y);
void ILI9341_Print(ILI9341 *disp, const char *text);
void ILI9341_PrintInteger(ILI9341 *disp, int num);
void ILI9341_PrintFloat(ILI9341 *disp, float value, uint8_t decimals);

// Оптимизированные функции
void ILI9341_DrawBufferDMA(
    ILI9341 *disp,
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t *buffer  // Чёткое указание типа
);

void ILI9341_DrawBuffer8to16(ILI9341 *disp, color8_t *active_buf8);

void ILI9341_SetAddressWindow(ILI9341 *disp, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);