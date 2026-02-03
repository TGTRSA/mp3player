
CC = g++
SOURCES = audio_player.cpp 
#SOURCES =  pcm.cpp
OUTPUT = audio_player
#OUTPUT = pcm
CFLAGS = -g -O0 -Wall -Wextra -I/usr/include -L/usr/lib -lavformat -lavcodec -lavutil -lasound 


all:
	$(CC) $(SOURCES) -o $(OUTPUT) $(CFLAGS) 

