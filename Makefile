# ==============================================================================
# Raylib C++ Makefile for Windows (w64devkit / MinGW)
# ==============================================================================

PROJECT_NAME ?= game
COMPILER_PATH ?= C:/raylib/w64devkit/bin
RAYLIB_PATH   ?= C:/raylib/raylib

CC = $(COMPILER_PATH)/g++

SRCS = src/main.cpp

INCLUDE_PATHS = -I. -I$(RAYLIB_PATH)/src -I$(RAYLIB_PATH)/src/external
LDFLAGS       = -L. -L$(RAYLIB_PATH)/src

LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm

CFLAGS = -std=c++17 -Wall -Wno-missing-braces -O2 $(INCLUDE_PATHS)

all:
	$(CC) $(SRCS) -o $(PROJECT_NAME).exe $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(PROJECT_NAME).exe
