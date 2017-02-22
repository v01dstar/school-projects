#include <stdio.h>

extern void a(char* a1, int a2, int a3);

extern void b(char* c);

int main() {
    a("1",1,1);
    b("123");
    return 0;
}
