CC=gcc
CXX=g++

#CFLAGS+=-Wall -g -fPIC
CPPFLAGS+=-Wall -g -fPIC -DNDEBUG
#LDFLAGS+=-shared -fPIC

INCLUDE=-I./ -I../
CPPFLAGS+=$(INCPATH)

objs+=json_writer.o json_value.o json_reader.o 

.PHONY:all
all:libjson.a testjson

libjson.a: $(objs)
	-mkdir -p ./lib
	ar -rcs ./lib/libjson.a $(objs)

json_writer.o: json_writer.cpp ./json/writer.h
	$(CXX) $(CPPFLAGS) $(INCLUDE) -c $<

json_reader.o: json_reader.cpp ./json/reader.h
	$(CXX) $(CPPFLAGS) $(INCLUDE) -c $<

json_value.o: json_value.cpp ./json/value.h
	$(CXX) $(CPPFLAGS) $(INCLUDE) -c $<
testjson:testjson.o libjson.a
	$(CXX) $(CFLAGS) -o $@ $< ./lib/libjson.a
testjson.o:testjson.cpp
	$(CXX) $(CPPFLAGS) $(INCLUDE) -c $<

.PHONY:clean
clean:
	rm -f *.o libjson.a testjson
