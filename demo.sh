#! /bin/sh

c++ --std=c++11 -Icpp cpp/ysl.cpp demo.cpp -o /tmp/demo -lglog -lyaml-cpp -lpthread

/tmp/demo 2>&1 | tee /tmp/demo.log & PYTHONPATH=$PYTHONPATH:python python3 demo.py
