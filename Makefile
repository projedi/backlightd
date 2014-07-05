.PHONY: all clean

TARGET := backlightd

SOURCES := backlightd.c io.c backlight.c
OBJECTS := $(SOURCES:.c=.o)
DEPENDS := $(SOURCES:.c=.d)

CC ?= gcc

CFLAGS += -g -std=c99 -Wall -Wextra -pthread $(shell pkg-config --cflags dbus-1)

LDFLAGS += -pthread $(shell pkg-config --libs-only-L dbus-1)

LIBS += -lm $(shell pkg-config --libs-only-l dbus-1)

all: $(TARGET)

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
