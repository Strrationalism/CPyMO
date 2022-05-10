.PHONY: build run clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo
BUILD_DIR_SDL2_BACKEND_MODULES := $(shell mkdir -p $(BUILD_DIR)/cpymo_backend_sdl2)$(BUILD_DIR)/cpymo_backend_sdl2
MODULES_FROM_SDL2_BACKEND := cpymo_backend_save.c cpymo_backend_font.c

OBJS := \
	$(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) \
	$(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c))) \
	$(patsubst %.c, $(BUILD_DIR_SDL2_BACKEND_MODULES)/%.o, $(MODULES_FROM_SDL2_BACKEND))

INC := $(wildcard *.h) $(wildcard ../../cpymo/*.h) $(wildcard ../include/*.h)

CC := cc -c
LD := cc

CFLAGS += \
	-DDISABLE_AUDIO \
	-DDISABLE_MOVIE \
	-DNON_VISUALLY_IMPAIRED_HELP \
	-DSCREEN_RESIZABLE \
	-DLOAD_GAME_ICON \
	-DTOGGLE_FULLSCREEN \
	-DDISABLE_SCALE \
	-DNDEBUG \
	-O0 -g \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include

LDFLAGS += -lm -lSDL -lSDLmain

ifneq ($(strip $(SDL)), "")
CFLAGS += -I$(SDL)/include
LDFLAGS += -L$(SDL)/lib
endif

ifeq ($(OS), Windows_NT)
CFLAGS += -m32
LDFLAGS += -m32
endif

TARGET := cpymo

build: $(TARGET)

run: build
	@./$(TARGET)

clean:
	@rm -rf build $(TARGET)

define compile
	@echo "$(notdir $1)"
	@$(CC) $1 -o $2 $(CFLAGS)
endef

$(BUILD_DIR_SDL2_BACKEND_MODULES)/%.o: ../sdl2/%.c
	$(call compile,$<,$@)

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c
	$(call compile,$<,$@)

$(BUILD_DIR)/%.o: %.c
	$(call compile,$<,$@)

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(LD) $^ -o $@ $(LDFLAGS) 
	@echo "=> $@"
