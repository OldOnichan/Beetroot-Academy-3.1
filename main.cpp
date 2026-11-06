#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t adc;
static void init_adc(adc_channel_t channel)
{
    adc_oneshot_unit_init_cfg_t unit = {};
    unit.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit, &adc));
    adc_oneshot_chan_cfg_t cfg = {};
    cfg.atten = ADC_ATTEN_DB_12;
    cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, channel, &cfg));
}
static int read_adc(adc_channel_t channel)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc, channel, &raw));
    return raw;
}

extern "C" void app_main()
{
    init_adc(ADC_CHANNEL_3); 
    gpio_reset_pin(GPIO_NUM_5);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    const int N = 16;
    const int DARK = 1700, LIGHT = 2500;
    int samples[N];
    int first = read_adc(ADC_CHANNEL_3);
    for (int &v : samples) v = first;
    int sum = first * N, index = 0;
    bool on = false;
    TickType_t wake = xTaskGetTickCount();
    while (true) {
        int raw = read_adc(ADC_CHANNEL_3);
        sum -= samples[index];
        samples[index] = raw;
        sum += raw;
        index = (index + 1) % N;
        int average = sum / N;
        if (average < DARK) on = true;
        else if (average > LIGHT) on = false;
        gpio_set_level(GPIO_NUM_5, on);
        printf("raw=%d sma=%d led=%d\n", raw, average, on);
        xTaskDelayUntil(&wake, pdMS_TO_TICKS(20));
    }
}
