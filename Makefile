##################
# Includes
##################

IGFILE_INCDIR = $(PWD)
XERCES_INCDIR = /opt/local/include
EVERYTHING_INCLUDE = -I$(IGFILE_INCDIR) -I$(XERCES_INCDIR)

##################
# Libraries
##################

XERCES_LIBS = -L/opt/local/lib -lxerces-c
EVERYTHING_LIBS = $(XERCES_LIBS)

##################
# Compilers
##################

CC = cc
CPP = /usr/bin/g++

##################
# Compiler options
##################

CPP_OPTIONS = -g

##################
# Suffix rules
##################

.SUFFIXES: .o .cc

.cc.o:
	@echo
	@echo  COMPILING $<
	@echo
	rm -f $@
	$(CPP) -c $(CPP_OPTIONS) $(EVERYTHING_INCLUDE) $<

MY_PROGRAM = xml2ig
MY_OBJECTS = $(MY_PROGRAM) IgTokenizer IgStyleParser IgParser IgLinearAlgebra IgCollection IgArchive

$(MY_PROGRAM) : $(MY_OBJECTS:=.o)
	@echo
	@echo Linking $@
	@echo  	
	@echo  	
	$(CPP) $(CPP_OPTIONS) -o $@ $(MY_OBJECTS:=.o) $(EVERYTHING_INCLUDE) $(EVERYTHING_LIBS)

clean :
	rm -f  *.o  
	rm -f  *.a
