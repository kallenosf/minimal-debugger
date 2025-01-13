#include <stdio.h>

void print_menu(){
    printf("\nOptions\n");
    printf("-------\n");
    printf("1. Calculate x + y\n");
    printf("2. Calculate x - y\n");
}

long addition(int x, int y){
    return x + y;
}

long subtraction(int x, int y){
    return x - y;
}

int main(){
    int x, y;
    int option;

    printf("Give me x: ");
    scanf("%u", &x);

    printf("Give me y: ");
    scanf("%u", &y);

    print_menu();

    printf("Choice: ");
    scanf("%d", &option);

    while (option != 1 && option != 2){
        printf("Your choice must be \"1\" or \"2\" ! \n");
        print_menu();
        printf("Choice: ");
        scanf("%d", &option);
    }

    if (option == 1)
        printf("x + y = %ld\n", addition(x,y));
    else
        printf("x - y = %ld\n", subtraction(x,y));


    return 0;
}

