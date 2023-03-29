#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include <regex.h>


///DEFINITIONS FOR RECIEVER BUFFER /////


static const int RX_BUF_SIZE = 2024; // buffer size to store the GPS DATA
static void get_String(char* str);
static void get_String2(char *str);

char arr1[] = "Latt";
char arr2[] = "Long";
char arr3[] = "           ";  //$GPGGA,002153.000,  (3342.6618,N) ,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E
char arr4[] = "            ";// $GPGGA,002153.000,3342.6618,N,  (11751.3858,W)  ,1,10,1.2,27.0,M,-34.2,M,,0000*5E  
//can extract more of the items from here by declaring char arr5 [NEED FULL SPACES]!

unsigned *msg_str1 = &arr1;
unsigned *msg_str2= &arr2;
unsigned *msg_str3 = &arr3;
unsigned *msg_str4 = &arr4;


#define TXD_PIN (GPIO_NUM_1) // UART 0 contain gpio number 1 and 3

// we recieve gps data on gpio 2

#define RXD_PIN (GPIO_NUM_3)
#define TXD_PIN2 (GPIO_NUM_17) // UART 2 PIN pin 26 in esp32
#define RXD_PIN2 (GPIO_NUM_16) // pin 25 in esp32
#define portTICK_RATE_MS 1
#define portTICK_PERIOD_MS 1

// FOR UART 0  
void init(void) 
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .source_clk = UART_SCLK_APB,

    };

    uart_driver_install(UART_NUM_0 , RX_BUF_SIZE*2 , 0 , 0 , NULL , 0);
    uart_param_config(UART_NUM_0 , &uart_config);
    uart_set_pin(UART_NUM_0 , TXD_PIN , RXD_PIN, UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);

}

//FOR UART 2

void init2(void)
{
    const uart_config_t uart_config_2 = {
        .baud_rate =9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, 
        .source_clk = UART_SCLK_APB,

    };
    uart_driver_install(UART_NUM_2 , RX_BUF_SIZE*2 , 0 , 0 , NULL , 0);
    uart_param_config(UART_NUM_2 , &uart_config_2);
    uart_set_pin(UART_NUM_2 , TXD_PIN2 , RXD_PIN2 , UART_PIN_NO_CHANGE , UART_PIN_NO_CHANGE);
     
}

static void rx_task(void *arg)
{

    const char *str = "$GPGGA";
    char *p = NULL;
    char *str1 [100];
    int i= 0;

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    char *str2 = (char*)malloc(sizeof(str1));
    while(true)
    {
        const int rxbytes = uart_read_bytes(UART_NUM_2, data , RX_BUF_SIZE , 1000/portTICK_RATE_MS);
        if (rxbytes > 0)
        {
            p=strstr((const char*) data , str);
            if(p)
            {
                for(int j = 0; p[j] != '\n'; j++)
                {
                    str2[i] = p[j];
                    i++;
                }
                str2[i+1] = '\n';
                const int len = i+2;
                data[rxbytes] = 0;
            uart_write_bytes(UART_NUM_0 , (const char *)str2 , len);
            get_String((char*)str2); // STRING PARSING
            get_String2((char*)str2); // USING REGEX
            }
           
            

        }
        vTaskDelay(100/portTICK_PERIOD_MS);
        
        
    }
    free(data);
    free(str2);
} 

void get_String(char* str)
{
    if (strlen(str) < 50) {
        printf("Input string is too short\n");
        return;
    }

    // Calculate the checksum of the received data
    unsigned char checksum = 0;
    for (int i = 0; str[i] != '*'; i++) {
        checksum ^= str[i];
    }

    // Convert the checksum value in the NMEA sentence to an integer
    char* checksum_str = strtok(str, "*");
    int checksum_value = (int)strtol(checksum_str + 1, NULL, 16);

    // Compare the calculated and provided checksum values
    if (checksum != checksum_value) {
        printf("Checksum mismatch\n");
        return;
    }

    // Extract latitude and longitude from the received data
    int i = 0;
    int count = 0;
    int k = 0;
    int n = 0;
    char new[50];
    while (count < 2) {
        if (str[i] == ',') {
            count++;
        }
        i++;
    }

    if (count != 2 || str[6] != '.' || str[16] != '.' || str[28] != ',' || str[33] != ',' || str[42] != ',') {
        printf("Wrong input format\n");
        return;
    }

    int j = 0;
    for (j = i; i + 24; j++) {
        new[k] = str[j];
        k++;
    }

    new[k] = '\n';
    while (new[n] != '\n') {
        printf("%c", new[n]);
        n++;
    }

    printf("%c", '\n');

    for (int x = n + 1; new[x] != '\n'; x++) {
        printf("%c", new[x]);
    }

    printf("%c", '\n');
}

//// USING REGEX METHOD ///////
void get_String2(char *str)
{
    char* pattern_str = "^[^,]*,[^,]*,([^,]*),([NS]),([^,]*),([EW]),.*"; //Used chatgpt to create the regex
    regex_t pattern;
    regmatch_t groups[5];

    if (regcomp(&pattern, pattern_str, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        return 1;
    }

    if (regexec(&pattern, str, 5, groups, 0)) {
        fprintf(stderr, "No match found\n");
        return 1;
    }

    char* latitude = malloc(groups[1].rm_eo - groups[1].rm_so + 2);
    strncpy(latitude, str + groups[1].rm_so, groups[1].rm_eo - groups[1].rm_so);
    latitude[groups[1].rm_eo - groups[1].rm_so] = groups[2].rm_so == -1 ? '\0' : str[groups[2].rm_so];

    char* longitude = malloc(groups[3].rm_eo - groups[3].rm_so + 2);
    strncpy(longitude, str + groups[3].rm_so, groups[3].rm_eo - groups[3].rm_so);
    longitude[groups[3].rm_eo - groups[3].rm_so] = groups[4].rm_so == -1 ? '\0' : str[groups[4].rm_so];

    printf("Latitude: %s\n", latitude);
    printf("Longitude: %s\n", longitude);

    free(latitude);
    free(longitude);
    regfree(&pattern);
}


//$GPGGA,002153.000,  (3342.6618,N) ,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E