
# ACHTUNG unbedingt TABS benutzen beim einrücken

CC = g++
#CFLAGS = -ggdb -w -pthread
CFLAGS = -Wall -O3 -std=c++17 -pthread -ffunction-sections -fdata-sections -lstdc++fs
LDFLAGS = -Wl,--gc-sections -lpthread -static-libgcc -static-libstdc++ -lstdc++fs # -latomic

INC_PATH = -I .
DIRS = SocketLib FastCgi tinyxml2 SrvLib
OBJLIBS = -l tinyxml2 -l fastcgi -l socketlib -l srvlib -l crypto -l ssl
LIB_PATH = -L tinyxml2 -L SocketLib -L FastCgi -L SrvLib

export CFLAGS
#VPATH = md5

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)

TARGET = webdav.exe

OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) md5.o

all: $(TARGET)

$(TARGET): $(BUILDDIRS) $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIB_PATH) $(OBJLIBS) $(LDFLAGS)

%.o: %.cpp
	@echo "Hallo Welt"
	$(CC) $(CFLAGS) $(INC_PATH) -c $<

md5.o:
	$(CC) $(CFLAGS) $(INC_PATH) -c md5/md5.cpp

$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

clean: $(CLEANDIRS)
	rm -f $(TARGET) $(OBJ) *~

$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: clean

