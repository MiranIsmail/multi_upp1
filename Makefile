CC = gcc
CFLAGS = -O2
LDFLAGS = -lpthread
TARGET1 = qsmm
TARGET2 = gaus
SRC1 = qsm.c
SRC2 = gaussian_multi.c

# Default target
all: $(TARGET1) $(TARGET2)

# Rule to build the target (executable)
$(TARGET1): $(SRC1)
					$(CC) $(CFLAGS) -o $(TARGET1) $(SRC1) $(LDFLAGS)


$(TARGET2): $(SRC2)
					$(CC) $(CFLAGS) -o $(TARGET2) $(SRC2) $(LDFLAGS)

tq:
	@echo "running quicksort"
	@time -f "%E	elapsed" ./$(TARGET1)


tg:
	@echo "running gaussian"
	@time -f "%E	elapsed" ./$(TARGET2)

ta: tq tg

# Clean up build files
.PHONY: clean
clean:
			rm -f $(TARGET1) $(TARGET2)

