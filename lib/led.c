#include "pico/stdlib.h"

void led_init(uint8_t pin){
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}