CC = g++
SOURCE_FILENAME = audio_player
SOURCE = $(SOURCE_FILENAME).cpp
OUTPUT = $(SOURCE_FILENAME).o

all:
	$(CC) $(SOURCE) -o $(OUTPUT) `pkg-config --cflags --libs libavformat libavcodec libavutil`
