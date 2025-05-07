#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "adc_driver/adc_driver.h"
#include "display_driver/display_driver.h"
#include "global_buffer/global_buffer.h"

int main() {
    stdio_init_all();

    // Инициализация периферии
    buffer_init();
    
    // Запуск ядра 1 с правильным указателем на функцию
    multicore_launch_core1(core0_adc_task);
    
    // Основной цикл ядра 0
    core1_display_task();
    
    return 0;
}