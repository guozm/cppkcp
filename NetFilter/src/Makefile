include ../../makefile.defs
SRCDIRS = .
SRCEXTS = .cpp

ifeq ($(OS), IOS)

SHARELIBS = libV2NetFilter.a
BUILD = ../Bin/$(SHARELIBS)
LIBDIR =
DEPLIBS = 

else

SHARELIB = libNetFilter.so
BUILD = -shared -fPIC -o $(BINDIR)/$(SHARELIB)
endif

INCLUDE= -I../../Net/inc/
BINDIR = ../../Bin

SOURCES = $(foreach d, $(SRCDIRS), $(wildcard $(addprefix $(d)/*, $(SRCEXTS))))
OBJS = $(foreach x, $(SRCEXTS), $(patsubst %$(x), %.o, $(filter %$(x), $(SOURCES))))

sharelib : $(OBJS) 
	$(LD) $(LDFLAGS) $(BUILD) $(OBJS) $(DEPLIBS)

$(OBJS) : %.o: %.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c $< -o $@ 

clean:
	rm -rf *.a *.o *.so
	rm -rf $(BINDIR)/$(SHARELIB)
