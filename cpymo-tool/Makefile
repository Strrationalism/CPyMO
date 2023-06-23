.PHONY: build clean

BUILD_DIR = $(shell mkdir -p build)build

INC := $(wildcard *.h) $(wildcard ../cpymo/*.h)

SRC_TOOL  := $(wildcard *.c)

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_TOOL))
CFLAGS := -O3 -DNDEBUG -DLEAKCHECK

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


$(BUILD_DIR)/%.o: %.c $(INC)
	$(call compile,$<,$@)

clean:
	@rm -rf $(BUILD_DIR) cpymo-tool
