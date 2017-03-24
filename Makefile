# Makefile for object GA Warrior
#RM = /bin/rm -f

TARGET = ff.out
CC = gcc
CFLAGS = -lglut -lGL -lGLU -lSDL

HEADERS = 
CFILES = forest_fire.c
XTRAF = 
OBJS = 

all: $(TARGET)

$(TARGET): $(HEADERS) $(CFILES) $(OBJS) $(XTRAF)
	$(CC) -o $(TARGET) $(CFILES) $(CFLAGS)


clean:
	$(RM) $(TARGET) $(OBJS) $(XTRAF) 

