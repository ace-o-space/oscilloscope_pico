#include "pico_ili9341/pico_ili9341.h"
#include "pico_ili9341/font_5x7.h"
#include "pico_ili9341/font_8x8.h"
#include "pico_ili9341/font_12x16.h"

// Текущие настройки шрифта
static const uint8_t *current_font;
static uint8_t font_width = 8;
static uint8_t font_height = 8;
static uint16_t text_color = 0xFFFF; // Белый
static uint16_t bg_color = 0x0000;   // Чёрный
static uint8_t text_size = 1;
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

// Установка текущего шрифта
void ILI9341_SetFont(ILI9341 *disp, const uint8_t *font) {
    current_font = font;
    
    if ((void *)font == (void *)font_5x7) {  // Сравнение через void*
        font_width = 5;
        font_height = 7;
    } else if ((void *)font == (void *)font_8x8) {
        font_width = 8;
        font_height = 8;
    } else if ((void *)font == (void *)font_12x16) {
        font_width = 12;
        font_height = 16;
    }
}

// Установка цвета текста и фона
void ILI9341_SetTextColor(ILI9341 *disp, uint16_t fg, uint16_t bg) {
    text_color = fg;
    bg_color = bg;
}

// Установка размера текста (масштабирование)
void ILI9341_SetTextSize(ILI9341 *disp, uint8_t size) {
    text_size = (size > 0) ? size : 1;
}

// Установка позиции курсора
void ILI9341_SetCursor(ILI9341 *disp, uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

// Рисование символа с текущими настройками
void ILI9341_DrawChar(ILI9341 *disp, char c) {
    if (c < 32 || c > 127) return; // Только печатные символы ASCII
    
    const uint8_t *char_data = &current_font[c * font_height];
    
    // Буфер для строки символа (оптимизация DMA)
    uint16_t pixel_buffer[font_width * font_height];
    uint16_t *ptr = pixel_buffer;
    
    // Преобразование битовой карты в пиксели
    for (uint8_t y = 0; y < font_height; y++) {
        uint8_t line = char_data[y];
        
        for (uint8_t x = 0; x < font_width; x++) {
            *ptr++ = (line & (1 << (font_width - 1 - x))) ? text_color : bg_color;
        }
    }
    
    // Масштабирование если нужно
    if (text_size > 1) {
        uint16_t scaled_buffer[font_width * text_size * font_height * text_size];
        uint16_t *dst = scaled_buffer;
        
        for (uint8_t y = 0; y < font_height; y++) {
            for (uint8_t ys = 0; ys < text_size; ys++) {
                for (uint8_t x = 0; x < font_width; x++) {
                    uint16_t color = pixel_buffer[y * font_width + x];
                    for (uint8_t xs = 0; xs < text_size; xs++) {
                        *dst++ = color;
                    }
                }
            }
        }
        
        // Вывод масштабированного символа
        ILI9341_DrawBufferDMA(disp, cursor_x, cursor_y, 
                             font_width * text_size, 
                             font_height * text_size, 
                             scaled_buffer);
    } else {
        // Вывод символа в оригинальном размере
        ILI9341_DrawBufferDMA(disp, cursor_x, cursor_y, 
                             font_width, 
                             font_height, 
                             pixel_buffer);
    }
    
    // Обновление позиции курсора
    cursor_x += font_width * text_size;
    
    // Перенос строки если нужно
    if (cursor_x > disp->width - font_width * text_size) {
        cursor_x = 0;
        cursor_y += font_height * text_size;
    }
}

// Вывод строки
void ILI9341_Print(ILI9341 *disp, const char *str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += font_height * text_size;
            str++;
        } else {
            ILI9341_DrawChar(disp, *str++);
        }
    }
}

//integer to string
void ILI9341_PrintInteger(ILI9341 *disp, int num) {
    char buffer[12]; // Достаточно для 32-битного int (-2147483648 до 2147483647)
    int i = 0;
    
    // Обработка отрицательных чисел
    if (num < 0) {
        ILI9341_DrawChar(disp, '-');
        num = -num;
    }
    // Обработка нуля отдельно
    else if (num == 0) {
        ILI9341_DrawChar(disp, '0');
        return;
    }
    
    // Преобразование числа в строку в обратном порядке
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    // Вывод цифр в правильном порядке
    while (i > 0) {
        ILI9341_DrawChar(disp, buffer[--i]);
    }
}

//вывод float
void ILI9341_PrintFloat(ILI9341 *disp, float value, uint8_t decimals) {
    int integer = (int)value;
    ILI9341_PrintInteger(disp, integer); // Вывод целой части
    
    if (decimals > 0) {
        ILI9341_Print(disp, ".");
        
        float frac = value - integer;
        for (uint8_t i = 0; i < decimals; i++) {
            frac *= 10;
            int digit = (int)frac;
            ILI9341_DrawChar(disp, '0' + digit);
            frac -= digit;
        }
    }
}

// Дополнительные функции для работы с текстом

// Вывод текста с заданными параметрами
void ILI9341_PrintEx(ILI9341 *disp, uint16_t x, uint16_t y, 
                    const uint8_t *font, uint8_t size,
                    uint16_t fg_color, uint16_t bg_color,
                    const char *str) {
    // Сохраняем текущие настройки
    const uint8_t *old_font = current_font;
    uint8_t old_size = text_size;
    uint16_t old_fg = text_color;
    uint16_t old_bg = bg_color;
    uint16_t old_x = cursor_x;
    uint16_t old_y = cursor_y;
    
    // Устанавливаем новые параметры
    ILI9341_SetFont(disp, font);
    ILI9341_SetTextSize(disp, size);
    ILI9341_SetTextColor(disp, fg_color, bg_color);
    ILI9341_SetCursor(disp, x, y);
    
    // Выводим текст
    ILI9341_Print(disp, str);
    
    // Восстанавливаем настройки
    current_font = old_font;
    text_size = old_size;
    text_color = old_fg;
    bg_color = old_bg;
    cursor_x = old_x;
    cursor_y = old_y;
}

// Получение ширины текста в пикселях
uint16_t ILI9341_GetTextWidth(const char *str) {
    uint16_t width = 0;
    
    while (*str) {
        if (*str == '\n') break;
        width += font_width * text_size;
        str++;
    }
    
    return width;
}

// Получение высоты текста в пикселях
uint16_t ILI9341_GetTextHeight() {
    return font_height * text_size;
}