#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#include "lib/button.h"
#include "lib/ssd1306.h"
#include "lib/led.h"

#define JOY_X 26
#define JOY_Y 27
#define JOY_BTN 22

#define BTN_A 5

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

#define DISPLAY_SDA 14
#define DISPLAY_SCL 15

#define ADC_WRAP 4096
#define CLK_DIV 16.0

#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1

#define DEAD_ZONE 200
#define CENTER_VALUE 2048

// Variável para manipular o display
uint8_t ssd[ssd1306_buffer_length];

// Prepara o canva do display
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// Time Track para debounce
static uint32_t last_time = 0;

// Variaveis de estado dos leds
static volatile bool led_r_state = false;
static volatile bool led_g_state = false;
static volatile bool led_b_state = false;

// Flag para alternar o estado da borda do display
static volatile bool display_border_alternate = false;

// Prototipos das funcoes
static void gpio_irq_handler(uint gpio, uint32_t events);
static bool debounce(uint32_t *last_time);
static uint setup_pwm(uint pwm_pin);
static uint16_t read_joystick_axis(uint16_t *vrx, uint16_t *vry);
static void setup();
static uint16_t adjust_value(uint16_t value);

// Varaiveis globais
uint slice_r;
uint slice_b;
uint16_t vrx_value;
uint16_t vry_value;

int main() {
    setup();

    while (true) {
        read_joystick_axis(&vrx_value, &vry_value);

        vrx_value = adjust_value(vrx_value);
        vry_value = adjust_value(vry_value);

        printf("Valor x: %d\nValor y: %d\n", vrx_value, vry_value);
        // Atualizando intensidade do LED Vermelho
        pwm_set_gpio_level(LED_R_PIN, vrx_value);
        // Atualizando intensidade do LED Azul
        pwm_set_gpio_level(LED_B_PIN, vry_value);

        // Verifica se o botao do joystick foi pressionado
        if (display_border_alternate) {
            /*code*/
        }
        sleep_ms(100);
    }
}

static void setup() {
    stdio_init_all();

    // Botoes
    button_init(JOY_BTN);
    button_init(BTN_A);

    //LEDs
    slice_r = setup_pwm(LED_R_PIN);
    slice_b = setup_pwm(LED_B_PIN);
    led_init(LED_G_PIN);

    // Inicializando joystick
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);

    //display
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);

    // Acionando interrupcoes
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOY_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
}

static uint setup_pwm(uint pwm_pin) {
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pwm_pin);

    pwm_set_clkdiv(slice, CLK_DIV);
    pwm_set_wrap(slice, ADC_WRAP);
    pwm_set_gpio_level(slice, 0);
    pwm_set_enabled(slice, true);

    //freq = 1,9 KHz

    return slice;
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (debounce(&last_time)) {
        // Desativando PWM dos LEDs vermelho e azul
        if(gpio == BTN_A) {
            printf(
                "Botao a pressionado\n"
                "Alternando estado dos PWMs\n"
            );

            led_r_state = !led_r_state;
            led_b_state = !led_b_state;

            pwm_set_enabled(slice_r, led_r_state);
            pwm_set_enabled(slice_b, led_b_state);
        } else {
            printf(
                    "Botao joy pressionado\n"
                    "Alternando estado do LED verde\n"
                    "Alternando borda\n"
                );

            led_g_state = !led_g_state;
            gpio_put(LED_G_PIN, led_g_state);
            
            display_border_alternate = true;
        }
    }
}

static bool debounce(uint32_t *last_time) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if(current_time - *last_time > 200000) {
        *last_time = current_time;
        return true;
    }
    return false;
}

static uint16_t read_joystick_axis(uint16_t *vrx, uint16_t *vry) {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vry = adc_read();

    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vrx = adc_read();
}

static uint16_t adjust_value(uint16_t value) {
    if (value > CENTER_VALUE + DEAD_ZONE) {
        return value - (CENTER_VALUE + DEAD_ZONE);
    } else if (value < CENTER_VALUE - DEAD_ZONE) {
        return (CENTER_VALUE - DEAD_ZONE) - value;
    } else {
        return 0;
    }
}