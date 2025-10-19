CC = gcc
INCLUDE = src 
LINK = m luajit-5.1
FLAGS = -std=c99 $(foreach INC,$(INCLUDE),-I$(INC))
LINK_FLAGS = $(foreach INC,$(LINK),-l$(INC))
BIN = doodle
DIR = build

$(DIR)/$(BIN): src/lua/main.c $(DIR)/doodle.o $(DIR)/lua.o
	$(CC) $(FLAGS) $^ -o $@ $(LINK_FLAGS)

$(DIR)/lua.o: src/lua/lua.c src/lua/lua.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR)/doodle.o: src/doodle/doodle.c src/doodle/doodle.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR):
	mkdir -p $(DIR)

clean:
	rm -r $(DIR)
