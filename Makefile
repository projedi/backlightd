.PHONY: all clean setuid-root

TARGET := backlight_helper

SOURCES := main.c
OBJECTS := $(SOURCES:.c=.o)
DEPENDS := $(SOURCES:.c=.d)

CC ?= gcc

CFLAGS += -std=c99 -Wall -Wextra

LDFLAGS +=

LIBS += -lm

all: setuid-root

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	$(CC) $(CFLAGS) -MF $@ -MG -MM -MP -MT $(^:.c=.o) $<

$(TARGET): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(OBJECTS) $(DEPENDS)

include $(DEPENDS)

setuid-root: $(TARGET)
	sudo chown root backlight_helper
	sudo chmod u+s backlight_helper
