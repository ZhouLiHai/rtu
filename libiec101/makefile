#Make file Create By Z

CC = arm-none-linux-gnueabi-gcc
AR = arm-none-linux-gnueabi-ar
OBJECTS = libiec101.o
HEADER = ../include

all: $(OBJECTS)
	$(AR) -crs libiec101.a $(OBJECTS) 
	cp libiec101.a ../libs

$(OBJECTS): %.o: %.c libiec101.h $(HEADER)/protocal.h
	$(CC) -std=gnu99 -I$(HEADER) -c $<

clean:
	rm -f *.o libiec101.a
