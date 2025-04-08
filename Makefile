# variables
CC = gcc
SRC_DIR = src
INCL_DIR = include
BLD_DIR = build
LIB_DIR = bin
TST_DIR = test
TST_BLD_DIR = $(BLD_DIR)/test

# base flags for all builds
BASE_FLAGS = -Wall -fPIC -Iinclude -Iinternal

# debug-specific flags
DBG_FLAGS = $(BASE_FLAGS) -g -DTSDL_DEBUG

# release-specific flags (libs)
REL_FLAGS = $(BASE_FLAGS) -O2 -flto

# test-specific flags
TST_CFLAGS = $(DBG_FLAGS)

# default linker config
LDFLAGS = -lsigcore -lglfw -lGL

# test-specific linker config
TST_LDFLAGS = $(LDFLAGS) -lsigtest -L/usr/lib

# X11 target sources
X11_OBJS = $(BLD_DIR)/tinysdl_x11.o
X11_DBG_CFLAGS = $(DBG_FLAGS) -DTSDL_BACKEND_X11
X11_REL_CFLAGS = $(REL_FLAGS) -DTSDL_BACKEND_X11
X11_LDFLAGS = -lsigcore -lX11 -lGL

# library build
LIB_SRCS = $(SRC_DIR)/tinysdl.c
LIB_OBJS = $(BLD_DIR)/tinysdl.o

# core build
CORE_OBJS = $(LIB_OBJS) $(BLD_DIR)/tinysdl_core.o

# mock build
MOCK_OBJS = $(BLD_DIR)/tinysdl_mock.o $(BLD_DIR)/tinysdl_mock_main.o

# main build
MAIN_OBJ = $(BLD_DIR)/main.o

# test case builds
TST_SRCS = $(wildcard $(TST_DIR)/test_*.c)
TST_OBJS = $(patsubst $(TST_DIR)/%.c, $(TST_BLD_DIR)/%.c.o, $(TST_SRCS))
TST_EXES = $(patsubst $(TST_DIR)/test_%.c, $(TST_BLD_DIR)/test_%, $(TST_SRCS))

# outputs
LIB_TARGET = $(LIB_DIR)/libtinysdl.so
EXE_TARGET = $(LIB_DIR)/tinysdl
MOCK_LIB_TARGET = $(LIB_DIR)/libtinysdl_mock.so
MOCK_EXE_TARGET = $(LIB_DIR)/tinysdl_mock

.PHONY: all clean mock run lib test_% exe x11 run_x11

# Default: debug build (GLFW executable and library)
all: CFLAGS = $(DBG_FLAGS)
all: $(LIB_TARGET) $(EXE_TARGET)

# X11 debug executable
x11: CFLAGS = $(X11_DBG_CFLAGS)
x11: $(LIB_DIR)/tsdl_x11

# Mock builds (debug)
mock: CFLAGS = $(DBG_FLAGS)
mock: $(MOCK_LIB_TARGET) $(MOCK_EXE_TARGET)

# GLFW library (release)
$(LIB_TARGET): CFLAGS = $(REL_FLAGS)
$(LIB_TARGET): $(CORE_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) -shared $(CORE_OBJS) -o $(LIB_TARGET) $(LDFLAGS)
	@strip $(LIB_TARGET)

# GLFW executable (debug)
$(EXE_TARGET): CFLAGS = $(DBG_FLAGS)
$(EXE_TARGET): $(CORE_OBJS) $(MAIN_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CC) $(CORE_OBJS) $(MAIN_OBJ) -o $(EXE_TARGET) $(LDFLAGS)

# Mock library (release)
$(MOCK_LIB_TARGET): CFLAGS = $(REL_FLAGS) -DTSDL_MOCK
$(MOCK_LIB_TARGET): $(MOCK_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) -shared $(MOCK_OBJS) -o $(MOCK_LIB_TARGET) $(LDFLAGS)
	@strip $(MOCK_LIB_TARGET)

# Mock executable (debug)
$(MOCK_EXE_TARGET): CFLAGS = $(DBG_FLAGS) -DTSDL_MOCK
$(MOCK_EXE_TARGET): $(MOCK_OBJS) $(MAIN_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CC) $(MOCK_OBJS) $(MAIN_OBJ) -o $(MOCK_EXE_TARGET) $(LDFLAGS)

# Core build rules
$(BLD_DIR)/tinysdl.o: $(SRC_DIR)/tinysdl.c $(INCL_DIR)/tinysdl.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD_DIR)/tinysdl_x11_main.o: $(SRC_DIR)/tinysdl.c $(INCL_DIR)/tinysdl.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(X11_DBG_CFLAGS) -c $< -o $@

$(BLD_DIR)/tinysdl_core.o: $(SRC_DIR)/tinysdl_core.c $(INCL_DIR)/tinysdl_core.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD_DIR)/main.o: $(SRC_DIR)/main.c $(INCL_DIR)/tinysdl.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# X11 build rules
$(LIB_DIR)/tinysdl_x11: CFLAGS = $(X11_REL_CFLAGS)
$(LIB_DIR)/tinysdl_x11: $(X11_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) -shared $(X11_OBJS) -o $@ $(X11_LDFLAGS)
	@strip $@

$(LIB_DIR)/libtinysdl_x11.so: CFLAGS = $(X11_REL_CFLAGS)
$(LIB_DIR)/libtinysdl_x11.so: $(X11_OBJS) $(BLD_DIR)/tinysdl.o
	@mkdir -p $(LIB_DIR)
	$(CC) -shared $(X11_OBJS) $(BLD_DIR)/tinysdl.o -o $@ $(X11_LDFLAGS)
	@strip $@

$(LIB_DIR)/tsdl_x11: CFLAGS = $(X11_DBG_CFLAGS)
$(LIB_DIR)/tsdl_x11: $(X11_OBJS) $(BLD_DIR)/tinysdl_x11_main.o $(MAIN_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CC) $(X11_OBJS) $(BLD_DIR)/tinysdl_x11_main.o $(MAIN_OBJ) -o $@ $(X11_LDFLAGS)

$(BLD_DIR)/tinysdl_x11.o: $(SRC_DIR)/X11/tinysdl_x11.c $(INCL_DIR)/tinysdl.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Mock build rules
$(BLD_DIR)/tinysdl_mock_main.o: $(SRC_DIR)/tinysdl.c $(INCL_DIR)/tinysdl.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -DTSDL_MOCK -c $< -o $@

$(BLD_DIR)/tinysdl_mock.o: $(SRC_DIR)/tinysdl_mock.c $(INCL_DIR)/tinysdl_mock.h
	@mkdir -p $(BLD_DIR)
	$(CC) $(CFLAGS) -DTSDL_MOCK -c $< -o $@

# Test build rules
$(TST_BLD_DIR)/%.c.o: $(TST_DIR)/%.c
	@mkdir -p $(TST_BLD_DIR)
	$(CC) $(TST_CFLAGS) -c $< -o $@

$(TST_BLD_DIR)/test_%: $(TST_BLD_DIR)/test_%.c.o $(LIB_OBJS)
	@mkdir -p $(TST_BLD_DIR)
	$(CC) $< $(LIB_OBJS) -o $@ $(TST_LDFLAGS)

test_%: $(TST_BLD_DIR)/test_%
	@$<

# Library install targets (release)
lib: $(LIB_TARGET)
	@sudo cp $(LIB_TARGET) /usr/local/lib/
	@sudo ldconfig

lib_x11: $(LIB_DIR)/libtinysdl_x11.so
	@sudo cp $(LIB_DIR)/libtinysdl_x11.so /usr/local/lib/
	@sudo ldconfig

# Clean
clean:
	find $(BLD_DIR) -type f -delete
	find $(LIB_DIR) -type f -delete

# Run targets (debug)
run: $(EXE_TARGET)
	@./$(EXE_TARGET)

run_x11: $(LIB_DIR)/tsdl_x11
	@./$<

# Debug executable
exe: CFLAGS = $(DBG_FLAGS)
exe: $(EXE_TARGET)
	@./$(EXE_TARGET)

exe: $(EXE_TARGET)
