#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nmea_parser.h"


#define NMEA_PARSER_RUNTIME_BUFFER_SIZE (CONFIG_NMEA_PARSER_RING_BUFFER_SIZE / 2)
#define NMEA_MAX_STATEMENT_ITEM_LENGTH (16)
#define NMEA_EVENT_LOOP_QUEUE_SIZE (16)


ESP_EVENT_DEFINE_BASE(ESP_NMEA_EVENT);

static const char *GPS_TAG = "nmea_parser";

typedef struct {
    uint8_t item_pos;                              /*!< Current position in item */
    uint8_t item_num;                              /*!< Current item number */
    uint8_t asterisk;                              /*!< Asterisk detected flag */
    uint8_t crc;                                   /*!< Calculated CRC value */
    uint8_t parsed_statement;                      /*!< OR'd of statements that have been parsed */
    uint8_t sat_num;                               /*!< Satellite number */
    uint8_t sat_count;                             /*!< Satellite count */
    uint8_t cur_statement;                         /*!< Current statement ID */
    uint32_t all_statements;                       /*!< All statements mask */
    char item_str[NMEA_MAX_STATEMENT_ITEM_LENGTH]; /*!< Current item */
    gps_t parent;                                  /*!< Parent class */
    uart_port_t uart_port;                         /*!< Uart port number */
    uint8_t *buffer;                               /*!< Runtime buffer */
    esp_event_loop_handle_t event_loop_hdl;        /*!< Event loop handle */
    TaskHandle_t tsk_hdl;                          /*!< NMEA Parser task handle */
    QueueHandle_t event_queue;                     /*!< UART event queue handle */
} esp_gps_t;


static float parse_lat_long(esp_gps_t *esp_gps)
{
    float ll = strtof(esp_gps->item_str, NULL);
    int deg = ((int)ll) / 100;
    float min = ll - (deg * 100);
    ll = deg + min / 60.0f;
    return ll;
}


static inline uint8_t convert_two_digit2number(const char *digit_char)
{
    return 10 * (digit_char[0] - '0') + (digit_char[1] - '0');
}


static void parse_utc_time(esp_gps_t *esp_gps)
{
    esp_gps->parent.tim.hour = convert_two_digit2number(esp_gps->item_str + 0);
    esp_gps->parent.tim.minute = convert_two_digit2number(esp_gps->item_str + 2);
    esp_gps->parent.tim.second = convert_two_digit2number(esp_gps->item_str + 4);
    if (esp_gps->item_str[6] == '.') {
        uint16_t tmp = 0;
        uint8_t i = 7;
        while (esp_gps->item_str[i]) {
            tmp = 10 * tmp + esp_gps->item_str[i] - '0';
            i++;
        }
        esp_gps->parent.tim.thousand = tmp;
    }
}



static esp_err_t parse_item(esp_gps_t *esp_gps)
{
    esp_err_t err = ESP_OK;
    /* start of a statement */
    if (esp_gps->item_num == 0 && esp_gps->item_str[0] == '$') {
        if (0) {
        }

        else {
            esp_gps->cur_statement = STATEMENT_UNKNOWN;
        }
        goto out;
    }
    /* Parse each item, depend on the type of the statement */
    if (esp_gps->cur_statement == STATEMENT_UNKNOWN) {
        goto out;
    }

    else {
        err =  ESP_FAIL;
    }
out:
    return err;
}


static esp_err_t gps_decode(esp_gps_t *esp_gps, size_t len)
{
    const uint8_t *d = esp_gps->buffer;
    while (*d) {
        /* Start of a statement */
        if (*d == '$') {
            /* Reset runtime information */
            esp_gps->asterisk = 0;
            esp_gps->item_num = 0;
            esp_gps->item_pos = 0;
            esp_gps->cur_statement = 0;
            esp_gps->crc = 0;
            esp_gps->sat_count = 0;
            esp_gps->sat_num = 0;
            /* Add character to item */
            esp_gps->item_str[esp_gps->item_pos++] = *d;
            esp_gps->item_str[esp_gps->item_pos] = '\0';
        }
        /* Detect item separator character */
        else if (*d == ',') {
            /* Parse current item */
            parse_item(esp_gps);
            /* Add character to CRC computation */
            esp_gps->crc ^= (uint8_t)(*d);
            /* Start with next item */
            esp_gps->item_pos = 0;
            esp_gps->item_str[0] = '\0';
            esp_gps->item_num++;
        }
        /* End of CRC computation */
        else if (*d == '*') {
            /* Parse current item */
            parse_item(esp_gps);
            /* Asterisk detected */
            esp_gps->asterisk = 1;
            /* Start with next item */
            esp_gps->item_pos = 0;
            esp_gps->item_str[0] = '\0';
            esp_gps->item_num++;
        }
        /* End of statement */
        else if (*d == '\r') {
            /* Convert received CRC from string (hex) to number */
            uint8_t crc = (uint8_t)strtol(esp_gps->item_str, NULL, 16);
            /* CRC passed */
            if (esp_gps->crc == crc) {
                switch (esp_gps->cur_statement) {

                default:
                    break;
                }
                /* Check if all statements have been parsed */
                if (((esp_gps->parsed_statement) & esp_gps->all_statements) == esp_gps->all_statements) {
                    esp_gps->parsed_statement = 0;
                    /* Send signal to notify that GPS information has been updated */
                    esp_event_post_to(esp_gps->event_loop_hdl, ESP_NMEA_EVENT, GPS_UPDATE,
                                      &(esp_gps->parent), sizeof(gps_t), 100 / portTICK_PERIOD_MS);
                }
            } else {
                ESP_LOGD(GPS_TAG, "CRC Error for statement:%s", esp_gps->buffer);
            }
            if (esp_gps->cur_statement == STATEMENT_UNKNOWN) {
                /* Send signal to notify that one unknown statement has been met */
                esp_event_post_to(esp_gps->event_loop_hdl, ESP_NMEA_EVENT, GPS_UNKNOWN,
                                  esp_gps->buffer, len, 100 / portTICK_PERIOD_MS);
            }
        }
        /* Other non-space character */
        else {
            if (!(esp_gps->asterisk)) {
                /* Add to CRC */
                esp_gps->crc ^= (uint8_t)(*d);
            }
            /* Add character to item */
            esp_gps->item_str[esp_gps->item_pos++] = *d;
            esp_gps->item_str[esp_gps->item_pos] = '\0';
        }
        /* Process next character */
        d++;
    }
    return ESP_OK;
}


static void esp_handle_uart_pattern(esp_gps_t *esp_gps)
{
    int pos = uart_pattern_pop_pos(esp_gps->uart_port);
    if (pos != -1) {
        /* read one line(include '\n') */
        int read_len = uart_read_bytes(esp_gps->uart_port, esp_gps->buffer, pos + 1, 100 / portTICK_PERIOD_MS);
        /* make sure the line is a standard string */
        esp_gps->buffer[read_len] = '\0';
        /* Send new line to handle */
        if (gps_decode(esp_gps, read_len + 1) != ESP_OK) {
            ESP_LOGW(GPS_TAG, "GPS decode line failed");
        }
    } else {
        ESP_LOGW(GPS_TAG, "Pattern Queue Size too small");
        uart_flush_input(esp_gps->uart_port);
    }
}


static void nmea_parser_task_entry(void *arg)
{
    esp_gps_t *esp_gps = (esp_gps_t *)arg;
    uart_event_t event;
    while (1) {
        if (xQueueReceive(esp_gps->event_queue, &event, pdMS_TO_TICKS(200))) {
            switch (event.type) {
            case UART_DATA:
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(GPS_TAG, "HW FIFO Overflow");
                uart_flush(esp_gps->uart_port);
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(GPS_TAG, "Ring Buffer Full");
                uart_flush(esp_gps->uart_port);
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BREAK:
                ESP_LOGW(GPS_TAG, "Rx Break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(GPS_TAG, "Parity Error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGE(GPS_TAG, "Frame Error");
                break;
            case UART_PATTERN_DET:
                esp_handle_uart_pattern(esp_gps);
                break;
            default:
                ESP_LOGW(GPS_TAG, "unknown uart event type: %d", event.type);
                break;
            }
        }
        /* Drive the event loop */
        esp_event_loop_run(esp_gps->event_loop_hdl, pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
}


nmea_parser_handle_t nmea_parser_init(const nmea_parser_config_t *config)
{
    esp_gps_t *esp_gps = calloc(1, sizeof(esp_gps_t));
    if (!esp_gps) {
        ESP_LOGE(GPS_TAG, "calloc memory for esp_fps failed");
        goto err_gps;
    }
    esp_gps->buffer = calloc(1, NMEA_PARSER_RUNTIME_BUFFER_SIZE);
    if (!esp_gps->buffer) {
        ESP_LOGE(GPS_TAG, "calloc memory for runtime buffer failed");
        goto err_buffer;
    }

    esp_gps->uart_port = config->uart.uart_port;
    esp_gps->all_statements &= 0xFE;
    /* Install UART friver */
    uart_config_t uart_config = {
        .baud_rate = config->uart.baud_rate,
        .data_bits = config->uart.data_bits,
        .parity = config->uart.parity,
        .stop_bits = config->uart.stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    if (uart_driver_install(esp_gps->uart_port, CONFIG_NMEA_PARSER_RING_BUFFER_SIZE, 0,
                            config->uart.event_queue_size, &esp_gps->event_queue, 0) != ESP_OK) {
        ESP_LOGE(GPS_TAG, "install uart driver failed");
        goto err_uart_install;
    }
    if (uart_param_config(esp_gps->uart_port, &uart_config) != ESP_OK) {
        ESP_LOGE(GPS_TAG, "config uart parameter failed");
        goto err_uart_config;
    }
    if (uart_set_pin(esp_gps->uart_port, UART_PIN_NO_CHANGE, config->uart.rx_pin,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(GPS_TAG, "config uart gpio failed");
        goto err_uart_config;
    }
    /* Set pattern interrupt, used to detect the end of a line */
    uart_enable_pattern_det_baud_intr(esp_gps->uart_port, '\n', 1, 9, 0, 0);
    /* Set pattern queue size */
    uart_pattern_queue_reset(esp_gps->uart_port, config->uart.event_queue_size);
    uart_flush(esp_gps->uart_port);
    /* Create Event loop */
    esp_event_loop_args_t loop_args = {
        .queue_size = NMEA_EVENT_LOOP_QUEUE_SIZE,
        .task_name = NULL
    };
    if (esp_event_loop_create(&loop_args, &esp_gps->event_loop_hdl) != ESP_OK) {
        ESP_LOGE(GPS_TAG, "create event loop faild");
        goto err_eloop;
    }
    /* Create NMEA Parser task */
    BaseType_t err = xTaskCreate(
                         nmea_parser_task_entry,
                         "nmea_parser",
                         CONFIG_NMEA_PARSER_TASK_STACK_SIZE,
                         esp_gps,
                         CONFIG_NMEA_PARSER_TASK_PRIORITY,
                         &esp_gps->tsk_hdl);
    if (err != pdTRUE) {
        ESP_LOGE(GPS_TAG, "create NMEA Parser task failed");
        goto err_task_create;
    }
    ESP_LOGI(GPS_TAG, "NMEA Parser init OK");
    return esp_gps;
    /*Error Handling*/
err_task_create:
    esp_event_loop_delete(esp_gps->event_loop_hdl);
err_eloop:
err_uart_install:
    uart_driver_delete(esp_gps->uart_port);
err_uart_config:
err_buffer:
    free(esp_gps->buffer);
err_gps:
    free(esp_gps);
    return NULL;
}


esp_err_t nmea_parser_deinit(nmea_parser_handle_t nmea_hdl)
{
    esp_gps_t *esp_gps = (esp_gps_t *)nmea_hdl;
    vTaskDelete(esp_gps->tsk_hdl);
    esp_event_loop_delete(esp_gps->event_loop_hdl);
    esp_err_t err = uart_driver_delete(esp_gps->uart_port);
    free(esp_gps->buffer);
    free(esp_gps);
    return err;
}


esp_err_t nmea_parser_add_handler(nmea_parser_handle_t nmea_hdl, esp_event_handler_t event_handler, void *handler_args)
{
    esp_gps_t *esp_gps = (esp_gps_t *)nmea_hdl;
    return esp_event_handler_register_with(esp_gps->event_loop_hdl, ESP_NMEA_EVENT, ESP_EVENT_ANY_ID,
                                           event_handler, handler_args);
}

esp_err_t nmea_parser_remove_handler(nmea_parser_handle_t nmea_hdl, esp_event_handler_t event_handler)
{
    esp_gps_t *esp_gps = (esp_gps_t *)nmea_hdl;
    return esp_event_handler_unregister_with(esp_gps->event_loop_hdl, ESP_NMEA_EVENT, ESP_EVENT_ANY_ID, event_handler);
}
