
CC = arm-none-linux-gnueabi-gcc

OBJECTS = main.o

CFLAGS = -Wl -mcpu=arm926ej -s

LIBS = -lrt -lpthread -lm

HEADER = ../include


all: $(OBJECTS)
	$(CC) $(LIBS) $(OBJECTS)  $(CFLAGS) -L../libs -lconfig -o dc
	cp dc ../bin_arm_dtu

$(OBJECTS): %.o: %.c
	$(CC) -std=gnu99 -finput-charset=UTF-8 -fexec-charset=GBK -I $(HEADER) -c $<

clean:
	rm -f *.o dc
