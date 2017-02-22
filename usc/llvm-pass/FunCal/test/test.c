#include <stdio.h>

void a(int a1, double a2, long a3) {
    printf("123\n");
}

void b(char b1, char *b2) {
    a(1, 2.2, 1);
    printf("123\n");
}

void c(short c1) {
    a(1, 2.2, 1);
    printf("123\n");
}


void d() {
    a(1, 2.2, 1);
    b('a', "abc");
    c(1);
    printf("123\n");
}

int main() {
    b('a', "abc");
}
