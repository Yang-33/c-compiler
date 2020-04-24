#!/bin/bash
assert(){
  expected="$1"
  input="$2"
  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 19 ' 5+ 20   - 4+1 -2-1 '

assert 3 '10+2-(3+4-(1-3))'

assert 11 '1+2*3+4'
assert 1 '1+60/3-2*(3*4-2)'
assert 10 '60/2/3'

assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'

assert 10 '-10+20'
assert 10 '- -10'
assert 10 '- - +10'
assert 10 '-10*(-5)+-40'

assert 0 '0==1'
assert 1 '42==42'
assert 1 '0!=1'
assert 0 '42!=42'

assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'

assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'

echo OK