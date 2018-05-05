
OUTPUT_DIR := bin
GPP := g++
SRC_DIR := src
INC_DIR := inc
BUILD_DIR := build
SRC_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

GPP_INC_DIRS := 
GPP_LIB_DIRS := 
GPP_LIBS := 

LIB_BINARY := $(OUTPUT_DIR)/libprofiler.a
LIB_GPPFLAGS := -g -Wall -Werror
LIB_LDFLAGS := -lrt -lpthread

TEST_SERVER := $(OUTPUT_DIR)/test_server
TEST_CLIENT := $(OUTPUT_DIR)/test_client
TEST_GPPFLAGS := -g -Wall -Werror
TEST_LDFLAGS := $(LIB_BINARY) -lrt -lpthread

QUIC_PROFILER := $(OUTPUT_DIR)/quic_profiler
QUIC_TEST_CLIENT := $(OUTPUT_DIR)/quic_test_client
QUIC_GPPFLAGS := -g -Wall -Werror
QUIC_LDFLAGS := $(LIB_BINARY) -lrt -lpthread

all: $(LIB_BINARY) $(TEST_CLIENT) $(TEST_SERVER) $(QUIC_PROFILER) $(QUIC_TEST_CLIENT)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(GPP) $(LIB_GPPFLAGS) $(GPP_INC_DIRS) -I$(INC_DIR) -MMD -MP -c -o $@ $< $(LIB_LDFLAGS)

$(LIB_BINARY): $(OBJ_FILES)
	@mkdir -p $(@D)
	ar rcs $@ $^

$(TEST_SERVER): test_server.cpp $(LIB_BINARY)
	@mkdir -p $(@D)
	$(GPP) $(TEST_GPPFLAGS) $(GPP_INC_DIRS) -I$(INC_DIR) -o $@ $< $(LIB_BINARY) $(TEST_LDFLAGS)

$(TEST_CLIENT): test_client.cpp $(LIB_BINARY)
	@mkdir -p $(@D)
	$(GPP) $(TEST_GPPFLAGS) $(GPP_INC_DIRS) -I$(INC_DIR) -o $@ $< $(TEST_LDFLAGS)

$(QUIC_PROFILER): src/quic-profiler/quic-profiler.cpp $(LIB_BINARY)
	@mkdir -p $(@D)
	$(GPP) $(QUIC_GPPFLAGS) $(GPP_INC_DIRS) -I$(INC_DIR) -o $@ $< $(LIB_BINARY) $(QUIC_LDFLAGS)

$(QUIC_TEST_CLIENT): src/quic-profiler/quic-test-client.cpp $(LIB_BINARY)
	@mkdir -p $(@D)
	$(GPP) $(QUIC_GPPFLAGS) $(GPP_INC_DIRS) -I$(INC_DIR) -o $@ $< $(LIB_BINARY) $(QUIC_LDFLAGS)

clean:
	rm -rf build/* bin/*
