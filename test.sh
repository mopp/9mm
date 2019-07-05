##!/bin/bash

try() {
    expected="$1"
    input="$2"

    echo "./9mm '$input'"
    ./9mm -str "$input" >tmp.s
    if [[ "$?" != "0" ]]; then
        echo 'Compilation error'
        exit 1
    fi

    gcc -no-pie -g -o tmp tmp.s src/lib.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo " -> $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 0  'int main() { 0; }'
try 42 'int main() { 42; }'
try 21 'int main() { 5+20-4; }'
try 41 'int main() {  12 + 34 - 5 ; }'
try 47 'int main() { 5+6*7; }'
try 15 'int main() { 5*(9-6); }'
try 4  'int main() { (3+5)/2; }'
try 4  'int main() { +4; }'
try 0  'int main() { 4 + -4; }'
try 1  'int main() { 10 == 10; }'
try 0  'int main() { 10 != 10; }'
try 1  'int main() { 10 <= 10; }'
try 1  'int main() { 10 >= 10; }'
try 0  'int main() { 10 > 10; }'
try 0  'int main() { 10 < 10; }'
try 1  'int main() { 1 + 2 == 3; }'
try 0  'int main() { 1 + 1 == 3; }'
try 1  'int main() { 3 == 1 + 2; }'
try 0  'int main() { 3 == 1 + 1; }'
try 1  'int main() { 1 + 2 <= 3; }'
try 0  'int main() { 1 + 2 < 3; }'
try 1  'int main() { 3 >= 1 + 2; }'
try 0  'int main() { 3 > 1 + 2; }'
try 1  'int main() { int a; a = 1; a; }'
try 3  'int main() { int a; int b; a = 1; b = 2; a+b; }'
try 1  'int main() { int a; int b; a = 1; b = 2; (a<b); }'
try 1  'int main() { return 1; }'
try 5  'int main() { return 5; return 8; }'
try 6  'int main() { int foo; int bar; foo = 1; bar = 2 + 3; return foo + bar; }'
try 30 'int main() { int x_d; int y_d; x_d = 10; y_d = 20; x_d+y_d; }'
try 3  'int main() { if (1) return 3; }'
try 0  'int main() { int foo; int bar; foo = 1; bar = 2; if (1) return 0; }'
try 1  'int main() { int foo; int bar; foo = 1; bar = 2; if (foo==1) return 1; }'
try 2  'int main() { int foo; int bar; foo = 1; bar = 2; if (foo==bar) return 1; else return 2; }'
try 0  'int main() { while (0) return 1; 0; }'
try 1  'int main() { while (1) return 1; 0; }'
try 3  'int main() { int i; i=0; while (i!=3) i=i+1; i; }'
try 1  'int main() { for (;;) return 1; }'
try 10 'int main() { int i; for (i=0; i<10; i=i+1) 1; return i; }'
try 1  'int main() { int i; for (i=0; i<10; i=i+1) return 1+i; return i; }'
try 45 'int main() { int i; int c; c=0; for (i=0; i<10; i=i+1) c = c+i; return c; }'
try 10 'int main() { int i; int c;c=0; for (i=0; i<10; i=i+1) c = c+1; return c; }'
try 1  'int main() { int c; if (1) {c=1; return c;} }'
try 9  'int main() { int i; for (i = 0; i<10; i=i+1) { if (i == 9) {return i;}} }'
try 9  'int main() { int i; for (i = 0; i<10; i=i+1) { if (i == 9) {return i;}} }'
try 0  'int main() { foo(); }'
try 1  'int main() { int i; for (i = 0; i<3; i=i+1) { foo(); } return 1; }'
try 3  'int main() { int a; int b; a = 1; b = 2; add(a, b); }'
try 0  'int main() { return 0; }'
try 2  'int main() { return bar(1, 1); } int bar(int x, int y) { return x + y;}'
try 0  'int main() { return fib(0); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 1  'int main() { return fib(1); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 1  'int main() { return fib(2); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 2  'int main() { return fib(3); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 3  'int main() { return fib(4); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 5  'int main() { return fib(5); } int fib(int n) { if (n <= 1) { return n; } return fib(n - 2) + fib(n - 1); }'
try 3  'int main() { return f(0); } int f(int n) { if (n == 3) { return n; } return f(n+1); }'
try 3  'int main() { int x; x=3; int* y; y=&x; return *y; }'
try 1  'int main() { int* p; alloc4(&p, 1, 2, 4, 8); return *p; }'
try 4  'int main() { int* p; alloc4(&p, 1, 2, 4, 8); int* q; q = p + 2; return *q; }'
try 8  'int main() { int* p; alloc4(&p, 1, 2, 4, 8); int* q; q = p + 2; return *(1+q); }'
try 8  'int main() { int* p; alloc4(&p, 1, 2, 4, 8); int* q; q = p + 2; return *(1+q); }'
try 0  'int main() { int* p; return (p+1)-(1+p)+(p-1)-(p-1); }'
try 0  'int main() { int* p; int** q; q = &p; return q-&p;}'
try 8  'int main() { int* p; int** q; q = &p; return (q+1)-&p;}'
try 0  'int main() { int* p; int** q; q = &p; return (q+1)-(&p+1);}'
try 4  'int main() { return sizeof(1);}'
try 4  'int main() { return sizeof(sizeof(4));}'
try 4  'int main() { int x; return sizeof(x);}'
try 4  'int main() { int x; return sizeof(x + 3);}'
try 8  'int main() { int* x; return sizeof(x);}'
try 8  'int main() { int* x; return sizeof(x + 1);}'
try 8  'int main() { int* x; return sizeof(*x);}'
try 0  'int main() { int a[10]; return 0;}'
try 1  'int main() { int a[10]; *a=1; return *a;}'
try 1  'int main() { int a[10]; *(a+0)=1; return *a;}'
try 1  'int main() { int a[10]; *a=1; int* p; p=a; return *p;}'
try 2  'int main() { int a[10]; *a=1; *(a+1)=2; int* p; p=a; return *(p+1);}'
try 1  'int main() { int a[10]; a[0]=1; return *a;}'
try 5  'int main() { int a[10]; a[0]=1; a[1]=3; a[2]=1; return a[0]+a[1]+a[2];}'
try 40 'int main() { int a[10]; return sizeof(a);}'
try 4  'int main() { int a[1]; return sizeof(a);}'
try 80 'int main() { int* a[10]; return sizeof(a);}'
try 8  'int main() { int* a[1]; return sizeof(a);}'
try 0  'int main() { int* a[4]; return a-&a[0];}'
try 2  'int x; int main() { x = 1; return x+1;}'
try 4  'int* foo[10]; int main() { foo[0] = 1; foo[9] = 3; return foo[0]+foo[9];}'
try 3  'int main() { char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; }'
try 97 'int main() { char* x; x = "abc"; return *x; }'
try 97 'int main() { char* x; x = "abc"; return x[0]; }'
try 0  'int main() { printf("hello, world\n"); return 0; }'
try 5  'int main() { int x; set(&x); return x;} int set(int* x) { *x=5; return 0; }'
try 8  'int main() { int x; int y; set(&x, &y); return x+y;} int set(int* x, int* y) { *x=4; *y=4; return 0;}'
