CC := g++
CFLAGS := -Wall -g -Os

OBJS = ftp.o main.o

all: ftp

ftp: $(OBJS)
	@$(CC) -o ftp $(CFLAGS) $(OBJS) -pthread

$(OBJS) : %.o: %.cpp 
	@$(CC) -c $(CFLAGS) $< -o $@ 

.PHONY:
clean:
	@rm -f *.o ftp
	@echo Done cleaning
