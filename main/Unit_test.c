#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "GPS_Module.h"



static const int RX_BUF_SIZE = 2024; // buffer size to store the GPS DATA
static void get_String(char* str);
static void get_String2(char *str);

char arr1[] = "Latt";
char arr2[] = "Long";
char arr3[] = "           ";  //$GPGGA,002153.000,  (3342.6618,N) ,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E
char arr4[] = "            ";// $GPGGA,002153.000,3342.6618,N,  (11751.3858,W)  ,1,10,1.2,27.0,M,-34.2,M,,0000*5E 

void test_get_String(void) {
    // Test case 1: Valid input string
    char input_str[] = "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E\n";
    char expected_output[] = "3342.6618";
    char output[50];
    get_String(input_str);
    strcpy(output, arr3);
    if (strcmp(output, expected_output) != 0) {
        printf("Test case 1 failed: expected '%s', but got '%s'\n", expected_output, output);
    }

    // Test case 2: Invalid input string (less than 48 characters)
    char input_str2[] = "$GPGGA,002153.000,3342.6618,N\n";
    get_String(input_str2);
    if (strlen(arr3) != 12) {
        printf("Test case 2 failed: expected length of output to be 12, but got %d\n", strlen(arr3));
    }

    // Test case 3: Invalid input string (wrong format)
    char input_str3[] = "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,0000*5E\n";
    get_String(input_str3);
    if (strlen(arr3) != 12) {
        printf("Test case 3 failed: expected length of output to be 12, but got %d\n", strlen(arr3));
    }
}

void test_get_String2(void) {
    // Test case 1: Valid input string
    char input_str[] = "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E\n";
    char expected_output[] = "3342.6618";
    char output[50];
    get_String2(input_str);
    strcpy(output, arr3);
    if (strcmp(output, expected_output) != 0) {
        printf("Test case 1 failed: expected '%s', but got '%s'\n", expected_output, output);
    }

    // Test case 2: Invalid input string (less than 48 characters)
    char input_str2[] = "$GPGGA,002153.000,3342.6618,N\n";
    get_String2(input_str2);
    if (strlen(arr3) != 12) {
        printf("Test case 2 failed: expected length of output to be 12, but got %d\n", strlen(arr3));
    }

    // Test case 3: Invalid input string (wrong format)
    char input_str3[] = "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,0000*5E\n";
    get_String2(input_str3);
    if (strlen(arr3) != 12) {
        printf("Test case 3 failed: expected length of output to be 12, but got %d\n", strlen(arr3));
    }
}

int main(void) {

    test_get_String();
    test_get_String2();
   
}