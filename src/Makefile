CXX = g++
CXXFLAGS = -std=c++17 -Wall $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0)
SRC = main.cpp steganography.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = stego_gui
all: $(TARGET)
$(TARGET): $(OBJ)
        $(CXX) -o $(TARGET) $(OBJ) $(LDFLAGS)
%.o: %.cpp
        $(CXX) $(CXXFLAGS) -c $< -o $@
clean:
        rm -f $(OBJ) $(TARGET)
