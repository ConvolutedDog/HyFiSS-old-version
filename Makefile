
USE_BOOST = 1
MPI_HOME = /usr/local/mpich-3.3.2
BOOST_HOME = /usr/local/boost
DEBUG = 1

MPICXX = $(MPI_HOME)/bin/mpic++
MPIRUN = $(MPI_HOME)/bin/mpirun

ifeq ($(USE_BOOST),1)
CXX = $(MPICXX)
else
CXX = g++
endif

CXXFLAGS = -Wall

# Detect Support for C++11 (C++0x) from GCC Version 
GNUC_CPP0X := $(shell g++ --version | perl -ne 'if (/g++\s+\(.*\)\s+([0-9.]+)/){ if($$1 >= 4.3) {$$n=1} else {$$n=0;} } END { print $$n; }')

ifeq ($(GNUC_CPP0X), 1)
	CXXFLAGS += -std=c++11
endif

CXXFLAGS += -I./ISA-Def -I./DEV-Def -I./trace-parser -I./trace-driven -I./common
CXXFLAGS += -I./common/CLI -I./common/CLI/impl -I$(MPI_HOME)/include
CXXFLAGS += -I$(BOOST_HOME)/include
CXXFLAGS += -I./parda
CXXFLAGS += $(shell pkg-config --cflags glib-2.0)

LIBRARIES = -L$(BOOST_HOME)/lib -lboost_mpi -lboost_serialization
LIBRARIES += $(shell pkg-config --libs glib-2.0)

ifeq ($(DEBUG),1)
OPTFLAGS = -O0 -g3 -fPIC
else
OPTFLAGS = -O3 -fPIC
endif

OBJ_PATH = obj


TARGET = memory_model.x


exist_OBJ_PATH = $(shell if [ -d $(OBJ_PATH) ]; then echo "exist"; else echo "noexist"; fi)

ifeq ("$(exist_OBJ_PATH)", "noexist")
$(shell mkdir $(OBJ_PATH))
endif

OBJS = $(OBJ_PATH)/splay.o $(OBJ_PATH)/process_args.o $(OBJ_PATH)/parda_print.o $(OBJ_PATH)/narray.o $(OBJ_PATH)/parda.o
OBJS += $(OBJ_PATH)/common_def.o $(OBJ_PATH)/option_parser.o $(OBJ_PATH)/trace-parser.o $(OBJ_PATH)/trace-driven.o $(OBJ_PATH)/main.o

default: $(TARGET)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(MPICXX) $(CXXFLAGS) $(OPTFLAGS) $(LIBRARIES) -o $@ $^

$(OBJ_PATH)/main.o: main.cc
	$(MPICXX) $(CXXFLAGS) $(OPTFLAGS) $(LIBRARIES) -o $@ -c $^

$(OBJ_PATH)/trace-parser.o: trace-parser/trace-parser.cc
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/trace-driven.o: trace-driven/trace-driven.cc
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/common_def.o: common/common_def.cc
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/option_parser.o: common/option_parser.cc
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/splay.o: parda/splay.c
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/process_args.o: parda/process_args.c
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/parda_print.o: parda/parda_print.c
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/parda.o: parda/parda.c
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

$(OBJ_PATH)/narray.o: parda/narray.c
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ -c $^

run:
	$(MPIRUN) -np 2 ./memory_model.x --configs ./traces/vectoradd/configs/ --sort 1 --log 0 > tmp.txt

.PHTONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -rf $(OBJ_PATH)