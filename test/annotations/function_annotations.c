#include <stdio.h>
#define SENSITIVE __attribute__((annotate("sensitive")))

void display() {

}

void calc() {

}

void notify() {
}

SENSITIVE
void readKey() {
    printf("Super Secret\n");
}

void encrypt() {
    readKey();
}

void decrypt() {
    readKey();
}

SENSITIVE
void loadUI() {
    display();
}

void operation1() {
    calc();
    notify();
}

void operation2() {
    encrypt();
    decrypt();
}

int main() {
    loadUI();
    operation1();
    operation2();
}
