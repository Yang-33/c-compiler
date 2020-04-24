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

echo OK