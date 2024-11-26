#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define DHT_GPIO CONFIG_EXAMPLE_DATA_GPIO

#if defined(CONFIG_EXAMPLE_TYPE_DHT11)
#define SENSOR_TYPE DHT_TYPE_DHT11
#elif defined(CONFIG_EXAMPLE_TYPE_AM2301)
#define SENSOR_TYPE DHT_TYPE_AM2301
#elif defined(CONFIG_EXAMPLE_TYPE_SI7021)
#define SENSOR_TYPE DHT_TYPE_SI7021
#endif

static const char *TAG = "MQTT_EXAMPLE";
esp_mqtt_client_handle_t mqtt_client;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    // TODO: Error handling
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}

void dht_collect(void *pvParameters) {
    float temperature, humidity;

#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);
#endif

    while (1) {
        esp_err_t result = dht_read_float_data(SENSOR_TYPE, DHT_GPIO, &humidity, &temperature);
        
        if (result == ESP_OK) {
            char temp_str[32];
            char hum_str[32];
            int temp_int = (int)temperature;
            int hum_int = (int)humidity;
            snprintf(temp_str, sizeof(temp_str), "%d", temp_int);
            snprintf(hum_str, sizeof(hum_str), "%d", hum_int);
            printf("Temperature: %dC - Humidity: %d\n", temp_int, hum_int);
            esp_mqtt_client_publish(mqtt_client, "mestrado/iot/aluno/khalil/temperatura", temp_str, 0, 1, 0);
            esp_mqtt_client_publish(mqtt_client, "mestrado/iot/aluno/khalil/umidade", hum_str, 0, 1, 0);
        } else {
            // Handle errors
            printf("Failed to read data from sensor. Error code: %d\n", result);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
    
    mqtt_app_start();
    
    xTaskCreate(dht_collect, "dht_collect", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
