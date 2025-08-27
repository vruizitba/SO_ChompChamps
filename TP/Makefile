# ----------------------------
# ChompChamps - Makefile (C99)
# ----------------------------

# Directorios
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
BUILD_DIR := build

# Toolchain
CC := gcc
CSTD := -std=c99
WARN := -Wall -Wextra -pedantic
OPT := -O2
CPPFLAGS := -I$(INC_DIR)
CFLAGS   := $(CSTD) $(WARN) $(OPT) $(CPPFLAGS)
LDFLAGS  :=
LDLIBS_COMMON := -pthread -lrt
LDLIBS_NCURSES := -lncurses

# Archivos comunes
COMMON_SRCS := $(SRC_DIR)/common.c $(SRC_DIR)/sync.c $(SRC_DIR)/util.c

# Fuentes
PLAYER_SRCS := $(SRC_DIR)/player.c $(SRC_DIR)/ai.c $(COMMON_SRCS)
VIEW_SRCS   := $(SRC_DIR)/view.c   $(COMMON_SRCS)
MASTER_SRCS := $(SRC_DIR)/master.c $(SRC_DIR)/args.c $(COMMON_SRCS)

# Binaries
PLAYER_BIN := $(BIN_DIR)/player
VIEW_BIN   := $(BIN_DIR)/view
MASTER_BIN := $(BIN_DIR)/master

# Objetos
PLAYER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(PLAYER_SRCS))
VIEW_OBJS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(VIEW_SRCS))
MASTER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(MASTER_SRCS))

# -------------- Targets --------------
.PHONY: all clean run deps check-ncurses docker-pull docker-shell docker-build docker-deps

all: $(PLAYER_BIN) $(VIEW_BIN) $(MASTER_BIN)

# Player
$(PLAYER_BIN): $(PLAYER_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS_COMMON)

# View
$(VIEW_BIN): $(VIEW_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS_COMMON) $(LDLIBS_NCURSES)

# Master
$(MASTER_BIN): $(MASTER_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS_COMMON)

# Objetos
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR) $(BUILD_DIR):
	@mkdir -p $@

clean:
	@echo "Limpieza..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

# ---------------- Ejecución ----------------
# Ejemplo: dos jugadores y la vista
run: #all
	$(MASTER_BIN) -w 20 -h 15 -d 200 -t 10 -p $(PLAYER_BIN) $(PLAYER_BIN) -v $(VIEW_BIN)

# ---------------- Dependencias ----------------
deps: check-ncurses
	@echo "Dependencias OK."

check-ncurses:
	@echo "Chequeando ncurses..."
	@printf '#include <ncurses.h>\nint main(){initscr();endwin();return 0;}\n' \
	  | $(CC) $(CSTD) -x c - -o /tmp/.ncurses_check -lncurses >/dev/null 2>&1 \
	&& { echo "  - ncurses OK"; rm -f /tmp/.ncurses_check; } \
	|| { echo "  - ncurses NO encontrado. Intentando instalar..."; \
	     if command -v apt-get >/dev/null 2>&1; then \
	       if command -v sudo >/dev/null 2>&1; then \
	          sudo apt-get update && sudo apt-get install -y libncurses5-dev libncursesw5-dev; \
	       else \
	          apt-get update && apt-get install -y libncurses5-dev libncursesw5-dev; \
	       fi; \
	     else \
	       echo "  * apt-get no disponible. Instalalo dentro del container (make docker-deps)."; \
	       exit 1; \
	     fi; \
	   }

# ---------------- Docker / Container ----------------
DOCKER_IMAGE := agodio/itba-so-multi-platform:3.0
WORKDIR := /work

docker-pull:
	docker pull $(DOCKER_IMAGE)

docker-shell:
	docker run --rm -it -v "$(PWD)":$(WORKDIR) -w $(WORKDIR) $(DOCKER_IMAGE) /bin/bash

docker-build:
	docker run --rm -it -v "$(PWD)":$(WORKDIR) -w $(WORKDIR) $(DOCKER_IMAGE) \
		/bin/bash -lc 'make clean all'

docker-deps:
	docker run --rm -it -v "$(PWD)":$(WORKDIR) -w $(WORKDIR) $(DOCKER_IMAGE) \
		/bin/bash -lc 'apt-get update && apt-get install -y libncurses5-dev libncursesw5-dev && make check-ncurses'
