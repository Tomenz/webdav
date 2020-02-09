
# ACHTUNG unbedingt TABS benutzen beim einrücken

CC = g++
#CFLAGS = -ggdb -w -D ZLIB_CONST -pthread
CFLAGS = -Wall -O3 -std=c++14 -pthread -ffunction-sections -fdata-sections -lstdc++fs
LDFLAGS = -Wl,--gc-sections -lpthread -static-libgcc -static-libstdc++ -lstdc++fs # -latomic

INC_PATH = -I ..

OBJLIBS = -l commonlib -l tinyxml2
LIB_PATH = -L ../CommonLib -L ../tinyxml2

TARGET = ../webdav.exe

OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIB_PATH) $(OBJLIBS) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INC_PATH) -c $<

clean:
	rm -f $(TARGET) $(OBJ) *~

