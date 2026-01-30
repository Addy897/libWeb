ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    EXEC_EXT = .exe
    RM = del /Q
else
    LIBS = -lpthread
    EXEC_EXT =
    RM = rm -f
endif

OBJS = mimeTypes.o routing.o request.o helper.o setupServer.o response.o hash_table.o

LIB = libWeb.a
CC = gcc
OUT_DIR = out

CFLAGS = -I"./include/" -ggdb -pthread
LIB_PATH = -L$(OUT_DIR) -lWeb

all: $(OUT_DIR)/$(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/$(LIB): $(OBJS)
	mkdir -p $(OUT_DIR)
	ar rcs $(OUT_DIR)/$(LIB) $(OBJS)

example: example/main.c $(OUT_DIR)/$(LIB)
	$(CC) $(CFLAGS) example/main.c $(LIB_PATH) $(LIBS) -o $(OUT_DIR)/main$(EXEC_EXT)

clean:
	$(RM) *.o $(OUT_DIR)/$(LIB) main$(EXEC_EXT)




