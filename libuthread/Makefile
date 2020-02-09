# Target library
lib := libuthread.a
all: $(lib)
objs := sem.o \
		#add tps.c later
mutedobjs := queue.o \
			 thread.o
# General gcc options
CC := gcc
CFLAGS	:= -Wall -Wextra -Werror
CFLAGS  += -g
# Include dependencies
deps := $(patsubst %.o,%.d,$(objs) $(mutedobjs))
-include $(deps)
# Generate dependencies
DEPFLAGS = -MMD -MF $(@:.o=.d)
# Rule for libuthread.a
libuthread.a: $(objs) $(mutedobjs)
	@echo "CC $@"
	ar rcs $@ $^
# Generic rule for compiling objects
%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)
# Cleaning rule
clean:
	@echo "CLEAN"
	rm -rf $(objs) $(deps)
