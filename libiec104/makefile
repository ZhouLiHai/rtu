#Make file Create By Z

CC = arm-none-linux-gnueabi-gcc
AR = arm-none-linux-gnueabi-ar
OBJECTS = libiec104.o
HEADER = ../include

all: $(OBJECTS)
	$(AR) -crs libiec104.a $(OBJECTS) 
	cp libiec104.a ../libs

$(OBJECTS): %.o: %.c libiec104.h $(HEADER)/protocal.h
	$(CC) -std=gnu99 -I$(HEADER) -c $<

clean:
	rm -f *.o libiec104.a
