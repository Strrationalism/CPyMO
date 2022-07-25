.PHONY: build run clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := \
	$(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) \
	$(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))

CFLAGS += \
	-DNDEBUG \
	-DDISABLE_AUDIO -DDISABLE_MOVIE \
	-DENABLE_TEXT_EXTRACT \
	-DLOW_FRAME_RATE \
	-O3 \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include

LDFLAGS += -O3 -lm

ifeq ($(OS), Windows_NT)
OBJS += $(BUILD_DIR)/cpymo.res
LDFLAGS += --static -lmingw32
endif

TARGET := cpymo-text

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
	@windres -i $< -o $@ -O coff

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "=> $@"
