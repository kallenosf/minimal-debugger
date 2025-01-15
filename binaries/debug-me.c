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
    int rcode;

    printf("Give me x: ");
    if (!scanf("%u", &x)){
        printf("x must be a number!\n");
        return 1;
    }

    printf("Give me y: ");
    if (!scanf("%u", &y)){
        printf("y must be a number!\n");
        return 1;
    }

    print_menu();

    rcode = scanf("%d", &option);

    while (!rcode || (option != 1 && option != 2)){
        if (!rcode)
            getchar();
        printf("Your choice must be \"1\" or \"2\" ! \n");
        print_menu();
        printf("Choice: ");
        rcode = scanf("%d", &option);
    }

    if (option == 1)
        printf("x + y = %ld\n", addition(x,y));
    else
        printf("x - y = %ld\n", subtraction(x,y));


    return 0;
}

