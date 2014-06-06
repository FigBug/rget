#
# Makefile: r
#
OBJS=block_allocator.o json.o mainheaders.o rget.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o ultragetopt.o url.o
CFLAGS=-g -Wall
CC=gcc
CXX=g++

# all
all: rget

# clean
clean: 
	rm -f $(OBJS) rget 

# rget
rget: $(OBJS)
	$(CXX) $(CFLAGS) -lcurl -framework CoreFoundation -framework ApplicationServices -o rget $(OBJS)

block_allocator.o: block_allocator.cpp
	$(CXX) $(CFLAGS) -c block_allocator.cpp

json.o: json.cpp
	$(CXX) $(CFLAGS) -c json.cpp

mainheaders.o: mainheaders.cpp
	$(CXX) $(CFLAGS) -c mainheaders.cpp

rget.o: rget.cpp
	$(CXX) $(CFLAGS) -c rget.cpp

tinystr.o: tinystr.cpp
	$(CXX) $(CFLAGS) -c tinystr.cpp

tinyxml.o: tinyxml.cpp
	$(CXX) $(CFLAGS) -c tinyxml.cpp

tinyxmlerror.o: tinyxmlerror.cpp
	$(CXX) $(CFLAGS) -c tinyxmlerror.cpp

tinyxmlparser.o: tinyxmlparser.cpp
	$(CXX) $(CFLAGS) -c tinyxmlparser.cpp

ultragetopt.o: ultragetopt.c
	$(CC) $(CFLAGS) -c ultragetopt.c

url.o: url.cpp
	$(CXX) $(CFLAGS) -c url.cpp


