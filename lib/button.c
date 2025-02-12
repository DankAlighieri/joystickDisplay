#include "pico/stdlib.h"

void button_init(uint pin){
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}