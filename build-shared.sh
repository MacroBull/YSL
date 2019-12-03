#! /bin/sh

set -x

c++ --std=c++11 -O2 -g -Icpp cpp/ysl.cpp -lglog -lyaml-cpp -lpthread -shared -fPIC -Wall -Wl,-soname,libysl.so.0 -o libysl.so.0
