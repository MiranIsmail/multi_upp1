CC = gcc
CFLAGS = -O2
LDFLAGS = -lpthread
TARGET1 = qsmm
TARGET2 = gaus
TARGET3 = qseq
TARGET4 = gseq
SRC1 = qsm.c
SRC2 = gaussianpar.c
SRC3 = qsortseq.c
SRC4 = gaussianseq.c

# Default target
all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

# Rule to build the target (executable)
$(TARGET1): $(SRC1)
					$(CC) $(CFLAGS) -o $(TARGET1) $(SRC1) $(LDFLAGS)


$(TARGET2): $(SRC2)
					$(CC) $(CFLAGS) -o $(TARGET2) $(SRC2) $(LDFLAGS)


$(TARGET3): $(SRC3)
					$(CC) $(CFLAGS) -o $(TARGET3) $(SRC3)


$(TARGET4): $(SRC4)
					$(CC) $(CFLAGS) -o $(TARGET4) $(SRC4)

tq:
	@echo "running quicksort"
	@time -f "%E	elapsed" ./$(TARGET1)


tg:
	@echo "running gaussian"
	@time -f "%E	elapsed" ./$(TARGET2)

dtg:
	@echo "running gaussian"
	@time -f "%E	elapsed" ./$(TARGET2) -n 16 -P 1
tgs:
	@echo "running gaussin seq"
	@time -f "%E	elapsed" ./$(TARGET4)

tqs:
	@echo "running quicksort seq"
	@time -f "%E	elapsed" ./$(TARGET3)

ta: tq tg

# Clean up build files
.PHONY: clean
clean:
			rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

