CC = cc
TARGET = $(BUILD)/lime
LIBS = -lm
BUILD= build/
CFLAGS = -g -Wall -fdiagnostics-color=always

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, $(BUILD)/%.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

$(BUILD)/%.o: %.c $(HEADERS)
	mkdir -p ${dir $@}
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

build: $(TARGET)

run: $(TARGET)
	./$(TARGET)

test:
	$(MAKE) -C tests test $(filter-out test,$(MAKECMDGOALS))

%:
	@:

clean:
	-rm -f -r $(BUILD)
	-rm -f $(TARGET)
