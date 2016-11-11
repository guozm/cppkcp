include ../../makefile.defs

ARCH:=$(shell uname -m)
BIT32:=i686
BIT64:=x86_64

SRCDIRS = .
SRCEXTS = .cpp


SHARELIBS = libNetEvent.a
BUILD = ../../Bin/$(SHARELIBS)
LIBDIR =  
SHARELIB = libNetEvent.so
BUILD = -shared -fPIC -o ../../Bin/$(SHARELIB)
LIBDIR = -L../../Bin/
DEPLIBS = -lNetFilter -levent -lkcp

BINDIR = ../../Bin
SOURCES = IOService.cpp KCPReadThread.cpp Mutex.cpp TCPWriteThread.cpp \
          BaseIOThread.cpp IoUtils.cpp KCPServer.cpp networkio.cpp \
		  KCPClient.cpp KCPThread.cpp Session.cpp \
		  kcpinc.cpp KCPWriteThread.cpp TCPReadThread.cpp
OBJS = $(foreach x, $(SRCEXTS), $(patsubst %$(x), %.o, $(filter %$(x), $(SOURCES))))

INCLUDE= -I../inc/ -I../../ikcp/

sharelib : $(OBJS) 
	$(LD) $(LDFLAGS) $(BUILD) $(OBJS) $(STATICLIBS) $(LIBDIR) $(DEPLIBS) 

$(OBJS) : %.o: %.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@ 

clean:
	rm -rf *.a *.o *.so
	rm -rf $(BINDIR)/$(SHARELIB)
