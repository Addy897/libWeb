ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    EXEC_EXT = .exe
    RM = del /Q
else
    LIBS = -lpthread
    EXEC_EXT =
    RM = rm -f
endif
LIB = libWeb.a

CC = gcc
OUT_DIR = out
SRC_DIR = ./src
EXAMPLE_DIR = ./example

PUBLIC_INC = ./include
PRIVATE_INC = ./src/internal

SRCS = mime_types.c routing.c helper.c setup_server.c connection.c request.c response.c json.c hash_table.c cache_store.c string_view.c

OBJS = $(SRCS:%.c=$(OUT_DIR)/%.o)

CFLAGS = -I$(PUBLIC_INC) -I$(PRIVATE_INC) -ggdb -O2 -fno-omit-frame-pointer -pthread -Wno-discarded-qualifiers
LIB_PATH = -L$(OUT_DIR) -lWeb


all: $(OUT_DIR)/$(LIB)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


PREFIX ?= ./example/Web

install: $(OUT_DIR)/$(LIB)
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/lib
	cp $(PUBLIC_INC)/*.h $(PREFIX)/include
	cp $(OUT_DIR)/$(LIB) $(PREFIX)/lib
$(OUT_DIR)/$(LIB): $(OBJS)
	mkdir -p $(OUT_DIR)
	ar rcs $(OUT_DIR)/$(LIB) $(OBJS)
example: $(EXAMPLE_DIR)/main.c $(OUT_DIR)/$(LIB)
	$(CC) $(CFLAGS) $(EXAMPLE_DIR)/main.c $(LIB_PATH) $(LIBS) -o $(EXAMPLE_DIR)/main$(EXEC_EXT)
.PHONY: clean
clean:
	$(RM) $(OUT_DIR)/*.o $(OUT_DIR)/$(LIB) main$(EXEC_EXT)


.PHONY: rebuild
rebuild: clean all


