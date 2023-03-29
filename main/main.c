#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "GPS_Module.h"




void app_main(void)

{
    init();
    init2();
   xTaskCreate(rx_task, "uart_rx_task", 1024*2 , NULL , configMAX_PRIORITIES,NULL);
   
}
