include ../makefile.defs
SRCDIRS = .
SRCEXTS = .cpp

ifeq ($(OS), IOS)

SHARELIBS = libkcp.a
BUILD =  ../Bin/$(SHARELIBS)
LIBDIR = 
DEPLIBS = 
else

SHARELIB = libkcp.so
BUILD = -shared -fPIC -o ../Bin/$(SHARELIB)
LIBDIR = 
DEPLIBS =
endif

SOURCES = ikcp.c
OBJS = ikcp.o

INCLUDE= 
sharelib : $(OBJS) 
	$(LD) $(LDLAGS) $(BUILD) $(LIBDIR) $(OBJS) $(DEPLIBS)

$(OBJS) : %.o: %.c
	gcc $(INCLUDE) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.a *.o *.so
	rm -rf ../Bin/$(SHARELIB)
