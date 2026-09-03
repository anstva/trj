CC := clang
CFLAGS ?= -g -Wall -Wextra
CPPFLAGS ?= -DCL_TARGET_OPENCL_VERSION=300
CPPFLAGS += -MMD -MP
LDFLAGS ?=
LDLIBS ?= -lOpenCL

TARGET := trj
SRCDIR := src

SRC := $(shell find $(SRCDIR) -type f -name '*.c')
OBJ := $(SRC:.c=.o)
DEP := $(OBJ:.o=.d)

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEP)

run: $(TARGET)
	./$(TARGET)

TEST_TARGET := test_sha256
TEST_SRC := tests/test_sha256.c

$(TEST_TARGET): $(TEST_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET) $(TEST_TARGET)

