CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
TARGET ?= mips-pipeline
SRC := src/main.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) --input inputs/test1.txt --output outputs/test1.txt

clean:
	rm -f $(TARGET) $(TARGET).exe
