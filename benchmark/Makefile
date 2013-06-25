SRC_DIR = ..
THREAD_DIR = $(SRC_DIR)/thread
MISC_DIR = $(SRC_DIR)/misc
SYS_DIR = $(SRC_DIR)/sys
OUT = output
LIBS = lib/libxthread.a


HEADER = -I./
HEADER += -I$(SRC_DIR)
HEADER += -I$(THREAD_DIR)
HEADER += -I$(SYS_DIR)
HEADER += -I$(MISC_DIR)

CXX = g++
CXXFLAGS += -g -Wall -Wextra
EXE = lockfreebenchmark

OBJECTS = $(OUT)/lockfreebenchmark.o

all : $(EXE)

$(OBJECTS): | $(OUT)

$(OUT):
	mkdir -p $(OUT)

$(OUT)/lockfreebenchmark.o : lockfreebenchmark.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c lockfreebenchmark.cc $(HEADER) -o $(OUT)/lockfreebenchmark.o

$(EXE) : $(OBJECTS)
	$(CXX) -o $(EXE) $(OBJECTS) $(SRC_DIR)/$(LIBS) -lpthread

clean :
	rm -f $(EXE) $(OUT)/*.o