OSYS = $(shell uname)

ifeq ($(OSYS),Darwin)
	FLAGS=`root-config --cflags` -I$(HOME)/.local/include -I../extern -O3 -g -mmacos-version-min=12.0 -fPIC
	COMPILER=clang++
endif
ifeq ($(OSYS),Linux)
	FLAGS=`root-config --cflags` -I$(HOME)/.local/include -I../extern -O3 -g -fPIC
	COMPILER=g++
endif

#define these for ROOT compilation
ROOTFLAGS = -L`root-config --libdir` `root-config --glibs`

all : libpixie.so

libpixie.so : measurement.o reader.o nsclreader.o event.o experiment_definition.o trace_algorithms.o pixie.hh traces.hh trace_algorithms.hh nsclreader.hh
		$(COMPILER) $(FLAGS) -shared -o ../lib/libpixie.so ../obj/measurement.o ../obj/event.o ../obj/reader.o ../obj/nsclreader.o ../obj/experiment_definition.o ../obj/trace_algorithms.o $(ROOTFLAGS)

measurement.o : measurement.cc measurement.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/measurement.o measurement.cc

reader.o : reader.cc reader.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/reader.o reader.cc

nsclreader.o : nsclreader.cc nsclreader.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/nsclreader.o nsclreader.cc

experiment_definition.o : experiment_definition.cc experiment_definition.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/experiment_definition.o experiment_definition.cc

#pre_reader.o : pre_reader.cc pre_reader.hh
#	$(COMPILER) $(FLAGS) -c -o pre_reader.o pre_reader.cc

trace_algorithms.o : trace_algorithms.cc trace_algorithms.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/trace_algorithms.o trace_algorithms.cc

event.o : event.cc event.hh
	$(COMPILER) $(FLAGS) -c -o ../obj/event.o event.cc

clean :
	rm ../obj/*.o
	rm ../lib/libpixie.so

#install dirs as appropriate
install :
	cp *.hh $(HOME)/.local/include/libpixie/
	cp ../lib/libpixie.so $(HOME)/.local/lib/
