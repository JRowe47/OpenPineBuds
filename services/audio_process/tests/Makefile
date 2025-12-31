CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror
CFLAGS += -I$(CURDIR)/.. -I$(CURDIR)/../.. \
          -I$(CURDIR)/../../multimedia/audio/process/filters/include \
          -I$(CURDIR)/../../../platform/hal \
          -I$(CURDIR)/../../../platform/hal/best2300p -DCHIP_BEST2300P
LDFLAGS ?=
LDLIBS ?= -lm

TARGET := audiogram_tests
SRCS := ../audiogram.c ../dsp_chain.c audiogram_tests.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS) $(LDLIBS)

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
