.PHONY: build clean

BUILD_DIR := $(shell mkdir -p build)build
BUILD_DIR_CPYMO := $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(wildcard *.c)) $(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(wildcard ../../cpymo/*.c)))
INC := $(wildcard *.h) $(wildcard ../../cpymo/*.h) $(wildcard ../include/*.h)

LDFLAGS := 
CFLAGS := \
	-DLIMIT_WINDOW_SIZE_TO_SCREEN \
	-DCPYMO_FEATURE_LEVEL=1 \
	-I../../lua \
	-DLINK_WITH_ONELUA

ifeq ($(DEBUG), 1)
CFLAGS += -DDEBUG -g
else
CFLAGS += -O3 -DNDEBUG
endif

ifeq ($(DISABLE_AUDIO), 1)
CFLAGS += -DDISABLE_AUDIO -DDISABLE_MOVIE
endif

ifeq ($(LEAKCHECK), 1)
CFLAGS += -DLEAKCHECK
endif

ifeq ($(ENABLE_EXIT_CONFIRM), 1)
CFLAGS += -DENABLE_EXIT_CONFIRM
endif

ifeq ($(OS), Windows_NT)
LDFLAGS += -lmingw32
endif

LDFLAGS += -lSDL2main -lSDL2 -lm

ifeq ($(DISABLE_AUDIO), 1)
CFLAGS += -DDISABLE_AUDIO -DDISABLE_MOVIE
endif

ifeq ($(DISABLE_VSYNC), 1)
CFLAGS += -DDISABLE_VSYNC
endif

ifeq ($(ENABLE_TEXT_EXTRACT_COPY_TO_CLIPBOARD), 1)
CFLAGS += -DENABLE_TEXT_EXTRACT -DENABLE_TEXT_EXTRACT_COPY_TO_CLIPBOARD
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

ifeq ($(NO_CONSOLE), 1)
LDFLAGS += -mwindows
endif

ifneq ($(strip $(RC_FILE)), "")
RC_FILE := ../sdl2/pymo-icon-windows.rc
endif

endif

$(BUILD_DIR)/cpymo.res: $(RC_FILE)
	@windres -i $< -o $@ -O coff

$(BUILD_DIR_CPYMO)/%.o: ../../cpymo/%.c $(INC)
	$(call compile,$<,$@)
$(BUILD_DIR)/%.o: %.c $(INC)
	$(call compile,$<,$@)

$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(CC) $^ $(LDFLAGS) -o $@
	@echo "=> $@"
