CC := g++
HDR := ./src ./src/core ./src/module ./src/cycle ./src/http
CFLAGS := -std=c++11 -Wall -g -DSERVX_DEBUG $(HDR:%=-I %)
# CFLAGS := -std=c++11 -Wall -O2 $(HDR:%=-I %)
SRC := $(wildcard ./src/*.cpp ./src/core/*.cpp ./src/module/*.cpp ./src/cycle/*.cpp ./src/http/*.cpp)
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
