##!/bin/bash

try() {
    expected="$1"
    input="$2"

    printf "./9mm '$input'"
    ./9mm "$input" >tmp.s
    gcc -g -o tmp tmp.s src/lib.o
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
