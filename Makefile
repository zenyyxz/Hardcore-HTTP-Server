CXX = g++
AS = gcc
CXXFLAGS = -nostdlib -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -Iinclude -Wall -O2
ASFLAGS = -c -nostdlib

SRC_CPP = src/main.cpp src/string_utils.cpp
SRC_ASM = src/start.S
OBJ = src/main.o src/start.o src/string_utils.o

TARGET = bin/http_server

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) -static

src/main.o: src/main.cpp include/syscalls.h include/types.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o src/main.o

src/string_utils.o: src/string_utils.cpp include/string_utils.h
	$(CXX) $(CXXFLAGS) -c src/string_utils.cpp -o src/string_utils.o

src/start.o: src/start.S
	$(AS) $(ASFLAGS) src/start.S -o src/start.o

clean:
	rm -f src/*.o $(TARGET)
