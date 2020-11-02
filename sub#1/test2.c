#include "sut.h"
#include <stdio.h>

void hello1() {
    int i;
    for (i = 0; i < 100; i++) {
	printf("Hello world!, this is SUT-One i = %d \n", i);
	sut_yield();
    }
    sut_exit();
}

void hello2() {
    int i;
    for (i = 0; i < 100; i++) {
	printf("Hello world!, this is SUT-Two i = %d \n", i);
	sut_yield();
    }
    sut_exit();
}

void hello3() {
    int i;
    for (i = 0; i < 100; i++) {
	printf("Hello world!, this is SUT-Three i = %d \n", i);
	sut_yield();
    }
    sut_exit();  
}

void hello4() {
    int i;
    for (i = 0; i < 100; i++) {
	printf("Hello world!, this is SUT-Four i = %d \n", i);
	sut_yield();
    }
    sut_exit();  
}

void hello5() {
    int i;
    for (i = 0; i < 100; i++) {
	printf("Hello world!, this is SUT-Five i = %d \n", i);
	sut_yield();
    }
    sut_exit();
}

int main() {
    sut_init();
    sut_create(hello1);
    sut_create(hello2);
    sut_create(hello3);
    sut_create(hello4);
    sut_create(hello5);  
    sut_shutdown();
}
