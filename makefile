CC = gcc
INCLUDE = src 
LINK = m luajit-5.1
FLAGS = -std=c99 $(foreach INC,$(INCLUDE),-I$(INC))
LINK_FLAGS = $(foreach INC,$(LINK),-l$(INC))
OBJ = doodle doodle_point lua lua_helpers lua_point
BIN = doodle
DIR = build

ifeq ($(debug), true)
	DIR = debug
	FLAGS += -g
endif

$(DIR)/$(BIN): src/lua/main.c $(foreach OB,$(OBJ),$(DIR)/$(OB).o)
	$(CC) $(FLAGS) $^ -o $@ $(LINK_FLAGS)

$(DIR)/lua.o: src/lua/lua.c src/lua/lua.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR)/lua_helpers.o: src/lua/lua_helpers.c src/lua/lua_helpers.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR)/lua_point.o: src/lua/lua_point.c src/lua/lua_point.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR)/doodle.o: src/doodle/doodle.c src/doodle/doodle.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR)/doodle_point.o: src/doodle/point.c src/doodle/point.h | $(DIR)
	$(CC) $(FLAGS) $< -c -o $@

$(DIR):
	mkdir -p $(DIR)

clean:
	rm -r $(DIR)
