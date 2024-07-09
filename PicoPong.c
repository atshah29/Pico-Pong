#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include <string.h>
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"



// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 12
#define I2C_SCL 13
#define I2C_ADDR 0x3C

#define POTENTIOMETER_PIN 26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BUZZER_PIN 16

void set_pwm_pin(uint pin, uint freq, uint duty_c) { // duty_c between 0..10000
		gpio_set_function(pin, GPIO_FUNC_PWM);
		uint slice_num = pwm_gpio_to_slice_num(pin);
    	pwm_config config = pwm_get_default_config();
		float div = (float)clock_get_hz(clk_sys) / (freq * 10000);
		pwm_config_set_clkdiv(&config, div);
		pwm_config_set_wrap(&config, 10000); 
		pwm_init(slice_num, &config, true); // start the pwm running according to the config
		pwm_set_gpio_level(pin, duty_c); //connect the pin to the pwm engine and set the on/off level.
        pwm_set_enabled(slice_num, true); 
	}

void playTone(uint pin, float frequency, int duration) {
    set_pwm_pin(pin, (uint)frequency, 5000); // 50% duty cycle
    sleep_ms(duration);
    set_pwm_pin(pin, (uint)frequency, 0); // Turn off the tone
    sleep_ms(50); // Short delay between notes
}



int main()
{
    stdio_init_all();
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c
    ssd1306_t display;
    display.external_vcc = false;
    ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, I2C_ADDR, I2C_PORT);
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, SCREEN_HEIGHT/2, 2, "Pico Pong!");
    ssd1306_show(&display);
    playTone(BUZZER_PIN, 261.63, 200); // C4
    playTone(BUZZER_PIN, 329.63, 200); // E4
    playTone(BUZZER_PIN, 392.00, 200); // G4
    playTone(BUZZER_PIN, 523.25, 500); // C5
    sleep_ms(5000);
    
    int paddle_y= SCREEN_HEIGHT/2;
    int ballx = SCREEN_WIDTH / 2;
    int bally = SCREEN_HEIGHT / 2;
    int ball_dx = 2;
    int ball_dy = 2;

    ssd1306_clear(&display);
    

    #define ball_size 4

    char scoreStr[20];
    int score=0;

    

    // Initialize currentPOT with the first reading
    

    while (true) {
        const float conversion_factor = 3.3f / (1 << 12);
        uint16_t newPOT = adc_read();

        paddle_y = (newPOT * (SCREEN_HEIGHT - 15 + 2)) /4096;

        if (paddle_y < 0) paddle_y = 0;
        if (paddle_y > SCREEN_HEIGHT - 15) paddle_y = SCREEN_HEIGHT - 15; //make sure paddle stays within bounds

        ballx+= ball_dx;
        bally+= ball_dy;
        
         if (ballx <= 0 || ballx >= SCREEN_WIDTH - ball_size) { // if ball hits left/right boundary
            ball_dx = -ball_dx;
         }

         if(bally <= 0 || bally >= SCREEN_HEIGHT- ball_size){ // if ball hits top/bottom boundary

            ball_dy= -ball_dy;
         }

         if (ballx <= 3 && bally >= paddle_y && bally <= paddle_y + 15) {
            score+=100;
            playTone(BUZZER_PIN, 500, 150 );
            set_pwm_pin(BUZZER_PIN, 500, 0);
            ball_dx = -ball_dx;
        }

        sprintf(scoreStr, "Score: %d", score);
        const char *scoreConstStr = scoreStr;

        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 10, 10, 1, scoreConstStr);
        ssd1306_draw_square(&display, 0, paddle_y, 3, 15);
        ssd1306_draw_square(&display, ballx, bally, ball_size, ball_size);
        ssd1306_show(&display);

        sleep_ms(20); // Reduced delay for smoother movement
    }
        
    }