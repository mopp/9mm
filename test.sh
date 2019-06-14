##!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./9mm "$input" >tmp.s
    gcc -o tmp tmp.s lib.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 0  '0;'
try 42 '42;'
try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'
try 47 '5+6*7;'
try 15 '5*(9-6);'
try 4  '(3+5)/2;'
try 4  '+4;'
try 0  '4 + -4;'
try 1  '10 == 10;'
try 0  '10 != 10;'
try 1  '10 <= 10;'
try 1  '10 >= 10;'
try 0  '10 > 10;'
try 0  '10 < 10;'
try 1  '1 + 2 == 3;'
try 0  '1 + 1 == 3;'
try 1  '3 == 1 + 2;'
try 0  '3 == 1 + 1;'
try 1  '1 + 2 <= 3;'
try 0  '1 + 2 < 3;'
try 1  '3 >= 1 + 2;'
try 0  '3 > 1 + 2;'
try 1  'a = 1; a;'
try 3  'a = 1; b = 2; a+b;'
try 1  'a = 1; b = 2; (a<b);'
try 1  'return 1;'
try 5  'return 5; return 8;'
try 6  'foo = 1; bar = 2 + 3; return foo + bar;'
try 30 'x_d = 10; y_d = 20; x_d+y_d;'
try 3 'if (1) return 3;'
try 0 'foo = 1; bar = 2; if (1) return 0;'
try 1 'foo = 1; bar = 2; if (foo==1) return 1;'
try 2 'foo = 1; bar = 2; if (foo==bar) return 1; else return 2;'
try 0 'while (0) return 1; 0;'
try 1 'while (1) return 1; 0;'
try 3 'i=0; while (i!=3) i=i+1; i;'
try 1 'for (;;) return 1;'
try 10 'for (i=0; i<10; i=i+1) 1; return i;'
try 1 'for (i=0; i<10; i=i+1) return 1+i; return i;'
try 45 'c=0; for (i=0; i<10; i=i+1) c = c+i; return c;'
try 10 'c=0; for (i=0; i<10; i=i+1) c = c+1; return c;'
try 1 'if (1) {c=1; return c;}'
try 9 'for (i = 0; i<10; i=i+1) { if (i == 9) {return i;}}'
try 9 'for (i = 0; i<10; i=i+1) { if (i == 9) {return i;}}'
try 0 'foo();'
try 1 'for (i = 0; i<3; i=i+1) { foo(); } return 1;'
try 3 'a = 1; b = 2; add(a, b);'
try 3 'a = 1; b = 2; add(a, b);'

echo OK
