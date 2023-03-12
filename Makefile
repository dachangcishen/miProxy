## CSCI 4430 Advanced Makefile

# How to use this Makefile...
###################
###################
##               ##
##  $ make help  ##
##               ##
###################
###################

CXX = gcc
# TODO For C++ only.
# CXXFLAGS = -std=c++11 -pedantic

all: miProxy

# TODO Modify source file name for your project.
# For C only.
 SOURCES = miProxy.c
# For C++ only.
# SOURCES = miProxy.cpp
miProxy: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o miProxy

clean:
	rm -rf miProxy *.dSYM

.PHONY: clean
