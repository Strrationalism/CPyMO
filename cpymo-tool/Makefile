.PHONY: build clean

BUILD_DIR = $(shell mkdir -p build)build
BUILD_DIR_CPYMO = $(shell mkdir -p $(BUILD_DIR)/cpymo)$(BUILD_DIR)/cpymo
BUILD_DIR_CPYMO_TOOL = $(shell mkdir -p $(BUILD_DIR)/cpymo-tool)$(BUILD_DIR)/cpymo-tool

INC := $(wildcard *.h) $(wildcard ../cpymo/*.h)

SRC_TOOL  := $(wildcard *.c)
SRC_CPYMO := \
	../cpymo/cpymo_error.c \
	../cpymo/cpymo_package.c \
	../cpymo/cpymo_parser.c \
	../cpymo/cpymo_utils.c \
	../cpymo/cpymo_color.c \
	../cpymo/cpymo_gameconfig.c \
	../cpymo/cpymo_assetloader.c \
	../cpymo/cpymo_string.c \
	../cpymo/cpymo_album.c

OBJS := $(patsubst %.c, $(BUILD_DIR_CPYMO_TOOL)/%.o, $(SRC_TOOL)) \
		$(patsubst %.c, $(BUILD_DIR_CPYMO)/%.o, $(notdir $(SRC_CPYMO)))

CFLAGS := -O3 -DNDEBUG -DCPYMO_TOOL -I../cpymo -I../stb -I../endianness.h -DLEAKCHECK

CC = cc -c
LD = cc

build: cpymo-tool
	@echo "=> $<"

define compile
	@echo "$(notdir $1)"
	@$(CC) $1 -o $2 $(CFLAGS)
endef

cpymo-tool: $(OBJS)
	@echo "linking..."
	@$(LD) $^ -o $@ -lm -O3

$(BUILD_DIR_CPYMO)/%.o: ../cpymo/%.c $(INC)
	$(call compile,$<,$@)

$(BUILD_DIR_CPYMO_TOOL)/%.o: %.c $(INC)
	$(call compile,$<,$@)

clean:
	@rm -rf $(BUILD_DIR) cpymo-tool
