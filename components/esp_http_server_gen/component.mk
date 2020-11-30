#component Makefile
SERVER_DATA_DIR    = $(PROJECT_PATH)/srv
GENERATED_CODE_DIR = $(PROJECT_PATH)/generated

COMPONENT_EMBED_FILES := $(shell $(COMPONENT_PATH)/gen_code.sh $(SERVER_DATA_DIR) $(GENERATED_CODE_DIR))
COMPONENT_EMBED_FILES := $(shell find $(SERVER_DATA_DIR) -type f)
#must be relative to COMPONENT_PATH
COMPONENT_SRCDIRS := . ../../generated
COMPONENT_OWNCLEANTARGET := 1

clean:
	rm -rf $(GENERATED_CODE_DIR)
