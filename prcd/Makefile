CC          =	gcc
CFLAGS      =	-O3 -g -Wall
INCLUDE     =	-I./ \
		-I./ \
		-I../ \
		-I../upUnixTools-1.0/include \
		-I../pflibs/libpifacedigital/src \
		-I../pflibs/libmcp23s17/src
DEFS   =
LIBS	 =	-L./ \
			-L../upUnixTools-1.0/lib -lupUnixTools -L../pflibs/libpifacedigital -lpifacedigital -L../pflibs/libmcp23s17 -lmcp23s17 
TARGET	    =  prcd
OBJS	    =   $(TARGET).o 

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(DEFS) $(INCLUDE) $(LIBS)


clean:
	rm -f core $(TARGET) *.o 
 
install:
	strip $(TARGET)
	cp $(TARGET) /opt/piracecam/bin
