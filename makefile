
mimeTypes = mimeTypes.o
routing = routing.o
parser = parser.o
helper = helper.o
setupServer = setupServer.o
httpResponse = httpResponse.o
LIB = libWeb.a
CC = gcc
ex_main= main.c
out= out

ifeq ($(OS),Windows_NT)
	CFLAGS = -lws2_32 -I"./include/"
	ex_CFLAGS= -l:../$(out)/libWeb.a -L$(out) -lws2_32
	ex_EXEC= main.exe
	Delete= del /s *.o 
else
	CFLAGS = -I"./include/"
	ex_EXEC= main
	ex_CFLAGS= -l:$(out)/libWeb.a -L".$(out)/"
endif

all: $(LIB)
$(mimeTypes): mimeTypes.c
	$(CC) $(CFLAGS) -c mimeTypes.c -o $(mimeTypes)
$(httpResponse): httpResponse.c
	$(CC) $(CFLAGS) -c httpResponse.c -o $(httpResponse)
$(setupServer): setupServer.c
	$(CC) $(CFLAGS) -c setupServer.c -o $(setupServer)
$(parser): parser.c
	$(CC) $(CFLAGS) -c parser.c -o $(parser)
$(routing): routing.c
	$(CC) $(CFLAGS) -c routing.c -o $(routing)
$(helper): helper.c
	$(CC) $(CFLAGS) -c helper.c -o $(helper)


$(LIB): $(mimeTypes) $(parser) $(helper) $(httpResponse) $(routing) $(setupServer)
	ld -relocatable $(mimeTypes) $(parser) $(helper) $(httpResponse) $(routing)  $(setupServer) -o web.o
	ar rcs $(out)/$(LIB) web.o
	
$(ex_EXEC): example/$(ex_main)
	$(CC) example/$(ex_main) -o example/$(ex_EXEC) $(ex_CFLAGS)

clean:
	$(Delete)

example:$(ex_EXEC)




