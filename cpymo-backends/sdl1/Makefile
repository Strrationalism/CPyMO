.PHONY: build run clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := \
	$(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) \
	$(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))

INC := $(wildcard *.h) $(wildcard ../../cpymo/*.h) $(wildcard ../include/*.h)

CC := cc -c
LD := cc

CFLAGS += \
	-DNON_VISUALLY_IMPAIRED_HELP \
	-DSCREEN_RESIZABLE \
	-DLOAD_GAME_ICON \
	-DTOGGLE_FULLSCREEN \
	-DDISABLE_SCALE \
	-DNDEBUG \
	-O3 \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include

ifneq ($(strip $(SDL)), "")
CFLAGS += -I$(SDL)/include
LDFLAGS += -L$(SDL)/lib
endif

ifneq ($(strip $(SDL_MIXER)), "")
CFLAGS += -I$(SDL_MIXER)/include
LDFLAGS += -L$(SDL_MIXER)/lib
endif

ifneq ($(strip $(FFMPEG)), "")
CFLAGS += -I$(SDL_FFMPEG)/include
LDFLAGS += -L$(FFMPEG)/lib
endif

ifeq ($(OS), Windows_NT)
CFLAGS += -m32
LDFLAGS += -m32 -lmingw32

ifeq ($(IMPORT_WINDOWS_ICON), 1)
OBJS += $(BUILD_DIR)/cpymo.res
endif
endif

LDFLAGS += -lSDLmain -lSDL -lm

ifeq ($(strip $(USE_FFMPEG)), 1)
LDFLAGS += -lswscale -lavformat -lavcodec -lavutil -lswresample
else

ifeq ($(strip $(USE_SDL_MIXER)), 1)
CFLAGS += -DDISABLE_MOVIE -DDISABLE_FFMPEG_AUDIO -DENABLE_SDL_MIXER_AUDIO_BACKEND
LDFLAGS += -lSDL_mixer
else
CFLAGS += -DDISABLE_AUDIO -DDISABLE_MOVIE
endif

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

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c
	$(call compile,$<,$@)

$(BUILD_DIR)/%.o: %.c
	$(call compile,$<,$@)

$(BUILD_DIR)/cpymo.res: ../sdl2/pymo-icon-windows.rc
	@windres $< $@

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(LD) $^ -o $@ $(LDFLAGS) 
	@echo "=> $@"
