.PHONY: build run clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := \
	$(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) \
	$(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))

INC := $(wildcard *.h) $(wildcard ../../cpymo/*.h) $(wildcard ../include/*.h)

CFLAGS += \
	-DDISABLE_AUDIO \
	-DDISABLE_MOVIE \
	-DNDEBUG \
	-O3 \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include

ifeq ($(LEAKCHECK), 1)
CFLAGS += -DLEAKCHECK
endif

LDFLAGS += -O3 -lm

TARGET := cpymo-ascii-art

build: $(TARGET)

run: build
	@./$(TARGET)

clean:
	@rm -rf build $(TARGET)

define compile
	@echo "$(notdir $1)"
	@$(CC) -c $1 -o $2 $(CFLAGS)
endef

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c
	$(call compile,$<,$@)

$(BUILD_DIR)/%.o: %.c
	$(call compile,$<,$@)

$(BUILD_DIR)/cpymo.res: ../sdl2/pymo-icon-windows.rc
	@windres $< $@ -O coff

ifeq ($(OS), Windows_NT)
OBJS += $(BUILD_DIR)/cpymo.res
LDFLAGS += --static
endif

$(TARGET): $(OBJS) $(WINDOWS_RES)
	@echo "Linking..."
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "=> $@"
