#! /bin/sh

c++ --std=c++11 -Icpp cpp/ysl.cpp -lglog -lyaml-cpp -lpthread -shared -fPIC -Wall -Wl,-soname,libysl.so.0 -o libysl.so.0
