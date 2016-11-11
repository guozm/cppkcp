include ../../../../makefile.defs
SRCDIRS = .
SRCEXTS = .cpp
EXE= test 

LIBDIR= ../../../../Lib

INCLUDE= -I$(LIBDIR)/V2_Protobuf/ -I$(LIBDIR)/V2_Log/inc/ -I$(LIBDIR)/ThreadPool/inc/ \
         -I$(LIBDIR)/Xml/inc/ -I$(LIBDIR)/XmlConfParse/inc/ -I../../inc/  -I../../../V2_Net/inc

SHARELIBDIR=

#SHARELIBS= $(BOOSTLIBS) $(V2LIBS) -lrt -luuid
#SHARELIBS= $(LIBS)

STATICLIBS= 

SOURCES = $(foreach d, $(SRCDIRS), $(wildcard $(addprefix $(d)/*, $(SRCEXTS))))
OBJS = $(foreach x, $(SRCEXTS), $(patsubst %$(x), %.o, $(filter %$(x), $(SOURCES))))

$(EXE): $(OBJS) $(STATICLIBS) $(SHARELIBS)
	$(CXX) $(INCLUDE) -o $@ $(OBJS) $(STATICLIBS) $(AM_CXXFLAGS) $(SHARELIBS) $(SHARELIBDIR) -lV2NetFilter

objs: $(OBJS)
%.o : %.cpp
	$(CXX) $(AM_CXXFLAGS) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ -pthread

clean:
	rm -rf *.o $(EXE)
	rm -rf log/*
