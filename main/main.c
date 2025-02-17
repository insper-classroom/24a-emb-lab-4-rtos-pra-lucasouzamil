#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

const int TRIG_PIN = 17;
const int ECHO_PIN = 16;

SemaphoreHandle_t xSemaphoreTrigger;
QueueHandle_t xQueueTime,xQueueDistance;

void gpio_callback(uint gpio, uint32_t events) {
    static uint32_t echo_rise_time, echo_fall_time, dt;
    if(events == 0x8) {  // rise edge
        if (gpio == ECHO_PIN) {
            echo_rise_time = to_us_since_boot(get_absolute_time());
        }
    } else if (events == 0x4) { // fall edge
        if (gpio == ECHO_PIN) {
            echo_fall_time = to_us_since_boot(get_absolute_time());
            dt = echo_fall_time-echo_rise_time;
            xQueueSendFromISR(xQueueTime, &dt, 0);
        }
    } 
}
/* 
void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}
 */
/* 
void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {

        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {

            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt,
                          27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112)
                cnt = 15;

            gfx_show(&disp);
        }
    }
}
 */
/* 
void oled1_demo_2(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);
    
    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init(); 

    while (1) {

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 2, "Batata");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 4, "Inhame");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}
*/
void trigger_task(void *p){
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);

    while (true){
        gpio_put(TRIG_PIN, 1);
        //busy_wait_us_32(10);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_put(TRIG_PIN, 0);
        xSemaphoreGive(xSemaphoreTrigger);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void echo_task(void *p){
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    
    const int V_SOM = 343000; // cm/s
    uint32_t dt;
    while (true) {
        if (xQueueReceive(xQueueTime, &dt,  0)) {
            double dif = (dt) / 10000000.0;
            double distance = (V_SOM * dif) / 2.0; 
            xQueueSend(xQueueDistance, &distance, 0);
        }
    }
}

void oled_task(void *p){
    
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);
   
    /* 
    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();  
    */

    double distance;
    char distance_str[20]; 
    while(true){
        if (xSemaphoreTake(xSemaphoreTrigger, 0) == pdTRUE) {
            if (xQueueReceive(xQueueDistance, &distance,  0)) {
                printf("Distancia: %f\n\n", distance);
                snprintf(distance_str, sizeof(distance_str), "%.2f cm", distance);

                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, "Distancia: ");
                gfx_draw_string(&disp, 0, 10, 1, distance_str);

                float scale = 1.5; // Mapeia 1 cm para 0.64 pixels
                int bar_length = (int)(distance * scale); // Aplica o fator de escala diretamente à distância em cm
                if(bar_length > 128) bar_length = 128; // Limita o comprimento da barra ao máximo da tela
                gfx_draw_line(&disp, 0, 20, bar_length, 20);
                gfx_show(&disp);
            } else {
                printf("Distancia: [FALHA]\n\n");

                // Limpar o buffer do display
                gfx_clear_buffer(&disp);

                // Desenhar o texto no buffer do display
                gfx_draw_string(&disp, 0, 0, 1, "Distancia: ");
                gfx_draw_string(&disp, 0, 10, 1, "[FALHA]");
                gfx_show(&disp);
            }
       } 
    }
}

int main() {
    stdio_init_all();

    xSemaphoreTrigger = xSemaphoreCreateBinary();
    xQueueTime = xQueueCreate(5, sizeof(uint32_t));
    xQueueDistance = xQueueCreate(5, sizeof(double));

    //xTaskCreate(oled1_demo_2, "Demo 2", 4095, NULL, 1, NULL);
    xTaskCreate(trigger_task, "Trigger task", 4095, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo task", 4095, NULL, 1, NULL);
    xTaskCreate(oled_task, "Oled task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
