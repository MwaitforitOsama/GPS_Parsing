ESP-IDF template app
====================

This is a template application to be used with [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).

Please check [ESP-IDF docs](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for getting started instructions.

*Code in this repository is in the Public Domain (or CC0 licensed, at your option.)
Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.*


To Work with this Library, you have to use three functions in your app_main code.
FUNCTIONS to call: 


init()
init2()
xTaskCreate(rx_task, "uart_rx_task", 1024*2 , NULL , configMAX_PRIORITIES,NULL);

//// Include "GPS_Module.h" in your main file

/// Included Unit_test.c file for unit testing!
