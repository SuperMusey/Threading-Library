override CFLAGS := -std=gnu99 -O0 -g $(CFLAGS) -I.
CC = gcc

# Build the .o file
threads.o: threads.c ec440threads.h
busy_threads.o: busy_threads.c ec440threads.h
thread_main.o: thread_main.c
				
# make executable
test_busy_threads: busy_threads.o threads.o
tmain: thread_main.c threads.o
	 $(CC) -o tmain threads.o thread_main.c

test_files=./test_busy_threads

.PHONY: clean check checkprogs all

# Build all of the test programs
checkprogs: $(test_files)

# Run the test programs
check: checkprogs
	tests/run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files) tmain