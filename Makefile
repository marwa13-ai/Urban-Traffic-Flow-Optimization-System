# ══════════════════════════════════════════════════════════
#  Makefile — Urban Traffic Flow Optimization System
#  Usage:
#    make          — build executable
#    make run      — build + run, print to stdout
#    make report   — build + run, save output to report.txt
#    make clean    — remove build artefacts
# ══════════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

TARGET  = traffic
SRCS    = main.c graph.c routing.c analytics.c
OBJS    = $(SRCS:.c=.o)

# ── Default target ──────────────────────────────────────
all: $(TARGET)
	@echo "Build successful → ./$(TARGET)"

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c graph.h
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Convenience targets ─────────────────────────────────
run: all
	./$(TARGET)

report: all
	./$(TARGET) > report.txt
	@echo "Output saved to report.txt"

# ── Cleanup ─────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(TARGET) report.txt

.PHONY: all run report clean
