#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flanger.h"

//fgets() and printf() user I/O
#define LEN_STR 80

//clears screen using ANSI escape sequences
void clear(){
    printf("\033[2J");
    printf("\033[%d;%dH", 0, 0);
}

void print_info(char *msg){
    //user I/O using printf() and getchar()
    clear();
    printf("Welcome to my flanger!\n"); //line 0
    printf("Enter rate and mix percentage below\n");
    printf("%s", msg);
    fflush(stdout);
}

int user_io_mix(){
    char in_str[LEN_STR]; //user input string
    print_info("Enter flanger mix percentage: ");

    //read input line, including trailing \n
    fgets(in_str, LEN_STR, stdin); //wait for input
    
    int user_mix = atoi(in_str);
    clear();

    return user_mix;
}

float user_io_rate(){
    char in_str[LEN_STR]; //user input string
    print_info("Enter flanger rate: ");

    //read input line, including trailing \n
    fgets(in_str, LEN_STR, stdin); //wait for input
    float user_rate = atof(in_str);
    clear();

    return user_rate;
}

float user_io_range(){
    char in_str[LEN_STR]; //user input string
    char *holder;
    float user_range;
    print_info("Enter flanger range: ");

    //read input line, including trailing \n
    fgets(in_str, LEN_STR, stdin); //wait for input

    holder = strtok(in_str,"\n");

    if (strcmp(holder,"low") == 0){
        user_range = 2.0;
    } else if (strcmp(holder,"mid") == 0){
        user_range = 1.0;
    } else if (strcmp(holder,"high") == 0){
        user_range = 0.5;
    } else {
        user_range = -1.0;
    }
    clear();

    return user_range;
}