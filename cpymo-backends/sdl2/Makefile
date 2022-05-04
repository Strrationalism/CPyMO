.PHONY: build clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) $(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))
INC := $(wildcard *.h) $(wildcard ../../cpymo.h)

CC := cc -c
LD := cc

CFLAGS := \
	-I../../cpymo \
	-I../../endianness.h \
	-I../../stb \
	-I../include \
	-I/usr/include/SDL2 \
	-O3 -DNDEBUG

LDFLAGS := -lSDL2 -lSDL2main -lm

ifeq ($(DISABLE_AUDIO), 1)
CFLAGS += -DDISABLE_AUDIO -DDISABLE_MOVIE
else

ifeq ($(USE_SDL2_MIXER), 1)
CFLAGS += -DDISABLE_MOVIE -DDISABLE_FFMPEG_AUDIO -DENABLE_SDL2_MIXER_AUDIO_BACKEND
LDFLAGS += -lSDL2_mixer
else
LDFLAGS += -lswscale -lavformat -lavcodec -lavutil -lswresample
endif

endif

ifdef $(SDL2)
CFLAGS += -I$(SDL2)/include/SDL2
LDFLAGS += -L$(SDL2)/lib
endif

ifdef $(SDL2_MIXER)
CFLAGS += -I$(SDL2_MIXER)/include/SDL2
LDFLAGS += -L$(SDL2_MIXER)/lib
endif

ifdef $(FFMPEG)
CFLAGS += -I$(FFMPEG)/include/SDL2
LDFLAGS += -L$(FFMPEG)/lib
endif

ifeq ($(NON_VISUALLY_IMPAIRED_HELP), 1)
CFLAGS += -DNON_VISUALLY_IMPAIRED_HELP
endif

ifndef SUFFIX
SUFFIX := 
endif

ifndef TARGET
TARGET := cpymo$(SUFFIX)
endif

build: $(TARGET)

clean:
	@rm -rf $(TARGET) $(BUILD_DIR) 

define compile
	@echo "$(notdir $1)"
	@$(CC) $1 -o $2 $(CFLAGS)
endef

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c $(INC)
	$(call compile,$<,$@)
$(BUILD_DIR)/%.o: %.c $(INC)
	$(call compile,$<,$@)

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(LD) $^ -o $@ $(LDFLAGS)
	@echo "=> $@"
