#include <stdio.h>
#include <string.h>
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

#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH 128

#define BORDER_HEIGHT 63
#define BORDER_WIDTH 127

// Variável para manipular o display
uint8_t ssd[ssd1306_buffer_length];

// Prepara o canva do display
struct render_area frame_area = {
    start_column : 0,
    end_column : BORDER_WIDTH,
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
static bool display_border_on = false;

/* 
    Medidas para manter o quadrado dentro do display
    display width - 8 - 1(Para nao tocar a borda nem sair 1 pixel) = 119
    display height - 8 - 1(Para nao tocar a borda nem sair 1 pixel) = 55
*/
static uint height_with_square = 55;
static uint width_with_square = 119;

// Prototipos das funcoes
static void gpio_irq_handler(uint gpio, uint32_t events);
static bool debounce(uint32_t *last_time);
static uint setup_pwm(uint pwm_pin);
static uint16_t read_joystick_axis(uint16_t *vrx, uint16_t *vry);
static void setup();
static uint16_t adjust_value(uint16_t value);
static void draw_square(uint x0, uint y0, bool set);
static void draw_border(bool set);
static int map(uint raw_coord_x, uint raw_coord_y);

// Varaiveis globais
uint slice_r, slice_b;
uint16_t vrx_value, vry_value;
uint x,y;

int main() {
    setup();

    int prev_x = 0;
    int prev_y = 0;

    while (true) {
        
        // Lendo os valores convertidos do joystick
        read_joystick_axis(&vrx_value, &vry_value);

        // Mapeando os valores do joystick para o display
        map(vrx_value, vry_value);

        // Ajustando os valores para o PWM
        vrx_value = adjust_value(vrx_value);
        vry_value = adjust_value(vry_value);

        // Atualizando intensidade do LED Vermelho
        pwm_set_gpio_level(LED_R_PIN, vrx_value);
        // Atualizando intensidade do LED Azul
        pwm_set_gpio_level(LED_B_PIN, vry_value);

        // Limpar o quadrado anterior
        draw_square(prev_x, prev_y, false);
        
        // Desenhar o quadrado no buffer
        draw_square(x,y, true);        

        // Checa se o botao do joystick foi pressionado
        if(display_border_alternate) {
            // Desenha a borda
            draw_border(display_border_on);
            display_border_alternate = !display_border_alternate;
        }  
        prev_x = x;
        prev_y = y;
    
        // Renderizar o buffer no display
        render_on_display(ssd, &frame_area);
        sleep_ms(100);
    }
}

void setup() {
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
    i2c_init(i2c1, 1000000);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);
    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Acionando interrupcoes
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(JOY_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_irq_handler);
}

static uint setup_pwm(uint pwm_pin) {
    gpio_set_function(pwm_pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pwm_pin);

    pwm_set_clkdiv(slice, CLK_DIV);
    pwm_set_wrap(slice, ADC_WRAP);
    pwm_set_gpio_level(pwm_pin, 0);
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
            
            display_border_alternate = !display_border_alternate;
            display_border_on = !display_border_on;
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

static void draw_square(uint x0, uint y0, bool set) {
    ssd1306_draw_line(ssd, x0, y0, x0 + 8, y0, set);
    ssd1306_draw_line(ssd, x0, y0, x0, y0 + 8, set);
    ssd1306_draw_line(ssd, x0, y0 + 8, x0 + 8, y0 + 8, set);
    ssd1306_draw_line(ssd, x0 + 8, y0, x0 + 8, y0 + 8, set);
}

static void draw_border(bool set) {
    ssd1306_draw_line(ssd, 0, 0, BORDER_WIDTH, 0, set);
    ssd1306_draw_line(ssd, 0, 0, 0, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, BORDER_WIDTH, 0, BORDER_WIDTH, BORDER_HEIGHT, set);
    ssd1306_draw_line(ssd, 0, BORDER_HEIGHT, BORDER_WIDTH, BORDER_HEIGHT, set);
}

static int map(uint raw_coord_x, uint raw_coord_y) {
    uint min_x = display_border_on ? 1 : 0; // Garante que o quadrado não toque a borda esquerda
    uint min_y = display_border_on ? 1 : 0; // Garante que o quadrado não toque a borda superior
    uint max_x = display_border_on ? width_with_square - 1 : width_with_square;
    uint max_y = display_border_on ? height_with_square - 1 : height_with_square;

    x = (raw_coord_x * max_x) / 4084;
    y = max_y - (raw_coord_y * max_y) / 4084; // Invertendo o eixo Y

    // Garantir que x e y não ultrapassem os limites
    if (x < min_x) x = min_x;
    if (y < min_y) y = min_y;
    if (x > max_x) x = max_x;
    if (y > max_y) x = max_y;
}