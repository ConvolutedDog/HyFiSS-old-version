
CC = g++

CXXFLAGS = -Wall

# Detect Support for C++11 (C++0x) from GCC Version 
GNUC_CPP0X := $(shell g++ --version | perl -ne 'if (/g++\s+\(.*\)\s+([0-9.]+)/){ if($$1 >= 4.3) {$$n=1} else {$$n=0;} } END { print $$n; }')

ifeq ($(GNUC_CPP0X), 1)
	CXXFLAGS += -std=c++0x
endif

CXXFLAGS += -I./ISA-Def -I./trace-parser -I./trace-driven -I./common
CXXFLAGS += -I./common/CLI -I./common/CLI/impl

OPTFLAGS = -O3 -g3 -fPIC

TARGET = memory_model.x

OBJ_PATH = obj

exist_OBJ_PATH = $(shell if [ -d $(OBJ_PATH) ]; then echo "exist"; else echo "noexist"; fi)

ifeq ("$(exist_OBJ_PATH)", "noexist")
$(shell mkdir $(OBJ_PATH))
endif

OBJS = $(OBJ_PATH)/common_def.o $(OBJ_PATH)/option_parser.o $(OBJ_PATH)/trace-parser.o $(OBJ_PATH)/trace-driven.o $(OBJ_PATH)/main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ $^

$(OBJ_PATH)/main.o: main.cc
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/trace-parser.o: trace-parser/trace-parser.cc
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/trace-driven.o: trace-driven/trace-driven.cc
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/common_def.o: common/common_def.cc
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/option_parser.o: common/option_parser.cc
	$(CC) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

.PHTONY: clean

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(OBJ_PATH)