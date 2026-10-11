# Compilateur et flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lm 

# Nom de l'exécutable
TARGET = simulator

# Fichiers sources
SRCS = main.c AWGN_Channel.c receiver.c transmitter.c

# Fichiers objets (générés automatiquement depuis SRCS)
OBJS = $(SRCS:.c=.o)

# Règle par défaut
all: $(TARGET)

# Lien final
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compilation de chaque .c en .o
%.o: %.c project.h
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJS) $(TARGET)
