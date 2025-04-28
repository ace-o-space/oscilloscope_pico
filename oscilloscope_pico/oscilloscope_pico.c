#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "adc_driver/adc_driver.h"
#include "display_driver/display_driver.h"
#include "global_buffer/global_buffer.h"

int main() {
    stdio_init_all();
    
    // Инициализация глобального буфера
    buffer_init();
    
    // Запуск обработки АЦП на ядре 1
    multicore_launch_core1(adc_processor_task);
    
    // Основной цикл на ядре 0
    display_task();
    
    return 0;
}