# ========= Config =========
IMG   := agodio/itba-so-multi-platform:3.0
NAME  := os-ram
MOUNT := /root
WORK  := $(MOUNT)    # compilo en /root (tu carpeta está montada ahí)

CC      := gcc
CSTD    := -std=c99
OPT     := -O2
WARN    := -Wall -Wextra -pedantic
CPPFLAGS:= -Iinclude
CFLAGS  := $(CSTD) $(OPT) $(WARN) $(CPPFLAGS)
LDLIBS  := -pthread -lrt
NCURSES := -lncurses

SRC_DIR := src
OUT_DIR := bin

COMMON_SRCS := $(SRC_DIR)/common.c $(SRC_DIR)/sync.c $(SRC_DIR)/util.c
MASTER_SRCS := $(SRC_DIR)/master.c $(COMMON_SRCS) $(wildcard $(SRC_DIR)/args.c)
PLAYER_SRCS := $(SRC_DIR)/player.c $(SRC_DIR)/ai.c $(COMMON_SRCS)
VIEW_SRCS   := $(SRC_DIR)/view.c   $(COMMON_SRCS)

MASTER_BIN := $(OUT_DIR)/master
PLAYER_BIN := $(OUT_DIR)/player
VIEW_BIN   := $(OUT_DIR)/view

# ========= Targets de alto nivel (siempre en host) =========
.PHONY: all master player view clean docker-pull docker-start host-all host-master host-player host-view

all: master player view

master: docker-start
	docker exec -it $(NAME) make -C $(WORK) host-master

player: docker-start
	docker exec -it $(NAME) make -C $(WORK) host-player

view: docker-start
	docker exec -it $(NAME) make -C $(WORK) host-view

clean:
	@rm -rf $(OUT_DIR)

# ======== Docker (persistente) ========
docker-pull:
	docker pull $(IMG)
	docker run -d --name $(NAME) --platform=linux/amd64 --privileged \
	    -v "$$PWD:$(MOUNT)" -w $(WORK) \
	    -it $(IMG) tail -f /dev/null
	docker exec -it $(NAME) apt install libncurses-dev

docker-start:
	docker start $(NAME)

# ========= Reglas "host-" (se ejecutan dentro del container) =========
# -> NO llaman a docker; compilan nativo usando gcc del container
host-all: host-master host-player host-view

host-master: $(MASTER_BIN)
host-player: $(PLAYER_BIN)
host-view:   $(VIEW_BIN)

$(OUT_DIR):
	@mkdir -p $@

$(MASTER_BIN): $(MASTER_SRCS) | $(OUT_DIR)
	$(CC) $(CFLAGS) -o $@ $(MASTER_SRCS) $(LDLIBS)

$(PLAYER_BIN): $(PLAYER_SRCS) | $(OUT_DIR)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_SRCS) $(LDLIBS)

$(VIEW_BIN): $(VIEW_SRCS) | $(OUT_DIR)
	$(CC) $(CFLAGS) -o $@ $(VIEW_SRCS) $(LDLIBS) $(NCURSES)
