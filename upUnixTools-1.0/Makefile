CC          =   gcc
CFLAGS      =   -O -Wall
INCLUDE     =   -I./include
RANLIB      =   ranlib
AR          =   ar
LN          =   ln
TARGET      =   upUnixTools
LIB         =   lib$(TARGET).a
OBJECTS     =   src/$(TARGET).o
 
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
 
all: $(TARGET)

$(TARGET): include/$(TARGET).h $(OBJECTS)
	$(AR) -cr lib/$(LIB) $(OBJECTS)
	$(RANLIB) lib/$(LIB)
 
clean:
	rm -f src/*.o lib/$(LIB) ../lib/$(LIB)
	
install:
	cp lib/$(LIB) ../lib	
