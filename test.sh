#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }
int add6(int a, int b, int c, int d, int e, int f){
    return a + b + c + d + e + f;
}
EOF

assert(){
  expected="$1"
  input="$2"
  ./y3c "$input" > tmp.s || exit
  cc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0  'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 19 'int main() { return  5+ 20   - 4+1 -2-1 ; }'

assert 3  'int main() { return 10+2-(3+4-(1-3)); }'

assert 11 'int main() { return 1+2*3+4; }'
assert 1  'int main() { return 1+60/3-2*(3*4-2); }'
assert 10 'int main() { return 60/2/3; }'

assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4  'int main() { return (3+5)/2; }'

assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'
assert 10 'int main() { return -10*(-5)+-40; }'

assert 0  'int main() { return 0==1; }'
assert 1  'int main() { return 42==42; }'
assert 1  'int main() { return 0!=1; }'
assert 0  'int main() { return 42!=42; }'

assert 1  'int main() { return 0<1; }'
assert 0  'int main() { return 1<1; }'
assert 0  'int main() { return 2<1; }'
assert 1  'int main() { return 0<=1; }'
assert 1  'int main() { return 1<=1; }'
assert 0  'int main() { return 2<=1; }'

assert 1  'int main() { return 1>0; }'
assert 0  'int main() { return 1>1; }'
assert 0  'int main() { return 1>2; }'
assert 1  'int main() { return 1>=0; }'
assert 1  'int main() { return 1>=1; }'
assert 0  'int main() { return 1>=2; }'

assert 1  'int main() { return 1; 2; 3; }'
assert 2  'int main() { 1; return 2; 3; }'
assert 3  'int main() { 1; 2; return 3; }'

assert 3  'int main() { int a; a=3; return a; }'
assert 3  'int main() { int a=3; return a; }'
assert 8  'int main() { int a=3; int z=5; return a+z; }'
assert 7  'int main() { int a,b; a=b=3; return a+b+1; }';

assert 3  'int main() { int foo=3; return foo; }'
assert 8  'int main() { int foo123=3; int bar=5; return foo123+bar; }'
assert 9  'int main() { int _a=3; int _b12=12; return -_a+_b12; }'

assert 2  'int main() { int a=1; if(0) return a; return 2*a; }'
assert 3  'int main() { if (0) return 2; return 3; }'
assert 3  'int main() { if (1-1) return 2; return 3; }'
assert 2  'int main() { if (1) return 2; return 3; }'
assert 2  'int main() { if (2-1) return 2; return 3; }'

assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3  'int main() { for (;;) return 3; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
assert 11 'int main() { int i=11; while(i<10) i=i+1; return i; }'

assert 3  'int main() { {1; {2;} return 3;} }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 14 'int main() { int i=0; if(1){ i = 7; return i*2; } return i; }'

assert 3  'int main() { int x=3; return *&x; }'
assert 3  'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5  'int main() { int x=3; int y=5; return *(&x+1); }'
assert 3  'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5  'int main() { int x=3; int *y=&x; *y=5; return x; }'

assert 7  'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7  'int main() { int x=3; int y=5; *(1+&x)=7; return y; }'
assert 7  'int main() { int x=3; int y=5; *(-1+&y)=7; return x; }'
assert 7  'int main() { int x=3; int y=5; *(&y-1)=7; return x; }'
assert 2  'int main() { int x=3; return (&x+2)-&x; }'

assert 8  'int main() { int x, y; x=3; y=5; return x+y; }'
assert 8  'int main() { int x=3, y=5; return x+y; }'

assert 3  'int main() { return ret3(); }'
assert 5  'int main() { return ret5(); }'

assert 8  'int main() { return add(3, 5); }'
assert 2  'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1, 2, 3, 4, 5, 6); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'

assert 7  'int main() { return add2(3, 4); } int add2(int x, int y) { return x + y; }'
assert 1  'int main() { return sub2(4, 3); } int sub2(int x, int y) { return x - y; }'
assert 34 'int main() { return fib(9); } int fib(int x) { if(x==0) return 0; if(x==1) return 1; return fib(x - 1) + fib(x - 2); }'

assert 3  'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

assert 3  'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4  'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5  'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 0  'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 1  'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
assert 2  'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
assert 3  'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
assert 4  'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5  'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'
assert 6  'int main() { int x[2][3]; int *y=x; *(y+6)=6; return **(x+2); }'

assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'

echo OK