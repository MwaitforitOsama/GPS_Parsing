#include "esp_log.h"
#include <string.h>
#include "stdlib.h"

#include "driver/uart.h"

#define GPS_UART_PORT       UART_NUM_2
#define GPS_UART_RX_PIN 16
#define GPS_UART_TX_PIN 17
#define TAG "gps_parser"
#define portTICK_RATE_MS 1



typedef struct {
    float latitude;
    float longitude;
    float altitude;
} gps_data_t;

static gps_data_t gps_data;

void parse_gps_data(const char* gps_string) {
    char* token;
    char* rest = (char*) gps_string;
    int i = 0;

    while ((token = strtok_r(rest, ",", &rest))) {
        if (i == 2) {
            gps_data.latitude = atof(token);
        } else if (i == 4) {
            gps_data.longitude = atof(token);
        } else if (i == 9) {
            gps_data.altitude = atof(token);
        }
        i++;
    }

    ESP_LOGI(TAG, "Parsed GPS data: lat=%f, long=%f, alt=%f", gps_data.latitude, gps_data.longitude, gps_data.altitude);
}

float get_indv_attr(const char* attr_name) {
    if (strcmp(attr_name, "lat") == 0) {
        return gps_data.latitude;
    } else if (strcmp(attr_name, "long") == 0) {
        return gps_data.longitude;
    } else if (strcmp(attr_name, "alt") == 0) {
        return gps_data.altitude;
    } else {
        ESP_LOGW(TAG, "Invalid attribute name");
        return 0.0f;
    }
}



void read_gps_data() {
    char gps_buffer[128];
    int gps_buffer_size = sizeof(gps_buffer);

    uart_config_t gps_uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(GPS_UART_PORT, &gps_uart_config);
    uart_set_pin(GPS_UART_PORT, GPS_UART_TX_PIN, GPS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_PORT, gps_buffer_size * 2, 0, 0, NULL, 0);

    while (1) {
        int len = uart_read_bytes(GPS_UART_PORT, (uint8_t*) gps_buffer, gps_buffer_size, 1000 / portTICK_RATE_MS);
        if (len > 0) {
            gps_buffer[len] = '\0';
            parse_gps_data(gps_buffer);
           
        }
    }
  
}




