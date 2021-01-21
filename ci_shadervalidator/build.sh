#!/bin/sh
g++ -std=c++17 -O2 -DSHADERVALIDATOR_STANDALONE ../ref/glad/src/glad.c ../gl/glsl_common.cpp ../gl/shader.cpp main.cpp -I../ref/glad/include -I../ref/glm -I. -o validateshaders
