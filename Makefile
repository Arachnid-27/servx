CC := g++
HDR := ./src ./src/core ./src/os ./src/module
CFLAGS := -std=c++11 -Wall -g $(HDR:%=-I %)
SRC := $(wildcard ./src/*.cpp ./src/core/*.cpp ./src/os/*.cpp ./src/module/*.cpp)
OBJ := $(SRC:%.cpp=%.o)
EXE := servx

$(EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(EXE) $(OBJ)

.PHONY: echo
echo:
	@echo $(SRC)
	@echo $(OBJ)
	@echo $(CFLAGS)
