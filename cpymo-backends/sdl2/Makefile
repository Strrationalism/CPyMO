.PHONY: build clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) $(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))
INC := $(wildcard *.h) $(wildcard ../../cpymo/*.h) $(wildcard ../include/*.h)

LDFLAGS := 
CFLAGS := \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include

ifeq ($(DEBUG), 1)
CFLAGS += -DDEBUG -g
else
CFLAGS += -O3 -DNDEBUG
endif

ifeq ($(OS), Windows_NT)
LDFLAGS += -lmingw32
endif

LDFLAGS += -lSDL2main -lSDL2 -lm

ifeq ($(DISABLE_AUDIO), 1)
CFLAGS += -DDISABLE_AUDIO -DDISABLE_MOVIE
endif

ifneq ($(strip $(SDL2)), )
CFLAGS += -I$(SDL2)/include/
LDFLAGS += -L$(SDL2)/lib/
endif

ifneq ($(strip $(SDL2_MIXER)), )
CFLAGS += -I$(SDL2_MIXER)/include/
LDFLAGS += -L$(SDL2_MIXER)/lib/
endif

ifneq ($(strip $(FFMPEG)), )
CFLAGS += -I$(FFMPEG)/include/
LDFLAGS += -L$(FFMPEG)/lib/
endif

ifndef TARGET
TARGET := cpymo
endif

ifneq ($(DISABLE_AUDIO), 1)
ifeq ($(USE_SDL2_MIXER), 1)
CFLAGS += -DDISABLE_MOVIE -DDISABLE_FFMPEG_AUDIO -DENABLE_SDL2_MIXER_AUDIO_BACKEND
LDFLAGS += -lSDL2_mixer
else
LDFLAGS += -lswscale -lavformat -lavcodec -lavutil -lswresample
endif
endif

build: $(TARGET)

clean:
	@rm -rf $(TARGET) $(BUILD_DIR) 

define compile
	@echo "$(notdir $1)"
	@$(CC) -c $1 -o $2 $(CFLAGS)
endef

ifeq ($(OS), Windows_NT)
OBJS += $(BUILD_DIR)/cpymo.res
LDFLAGS += -lwinpthread
endif

$(BUILD_DIR)/cpymo.res: ../sdl2/pymo-icon-windows.rc
	@windres -i $< -o $@ -O coff

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c $(INC)
	$(call compile,$<,$@)
$(BUILD_DIR)/%.o: %.c $(INC)
	$(call compile,$<,$@)

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(CC) $^ $(LDFLAGS) -o $@
	@echo "=> $@"
