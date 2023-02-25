CC = g++
TARGET = main
CFLAGS = -lOpenMeshCore -lOpenMeshTools -lGL -lGLU -lGLEW -lglut -lm -Wall -Og
OUT = out
SRC = src
SRCS = $(wildcard $(SRC)/*.cpp)
OBJS = $(SRCS:$(SRC)/%.cpp=$(OUT)/%.o)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $^

$(OUT)/%.o : $(SRC)/%.cpp | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT):
	mkdir $(OUT)

clean:
	rm -rf out
	rm main

.DEFAULT: run
.PHONY: run