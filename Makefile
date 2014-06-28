.PHONY: all clean

TARGET := backlightd

SOURCES := main.c io.c backlight.c
OBJECTS := $(SOURCES:.c=.o)
DEPENDS := $(SOURCES:.c=.d)

CC ?= gcc

CFLAGS += -std=c99 -Wall -Wextra

LDFLAGS +=

LIBS += -lm

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
