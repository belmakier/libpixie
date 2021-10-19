FLAGS = -std=c++17 -O3
COMPILER=clang++  #make this g++ if linux

#define these for ROOT compilation
#FLAGS = -std=c++17 -I`root-config --incdir` -I../extern -O3 -g
#ROOTFLAGS = -L`root-config --libdir` `root-config --glibs --new `

all : libpixie.so

libpixie.so : measurement.o reader.o event.o experiment_definition.o trace_algorithms.o pixie.hh traces.hh trace_algorithms.hh
		$(COMPILER) $(FLAGS) -shared -o ../lib/libpixie.so ../obj/measurement.o ../obj/event.o ../obj/reader.o ../obj/experiment_definition.o ../obj/trace_algorithms.o $(ROOTFLAGS)

measurement.o : measurement.cc measurement.hh
	$(COMPILER) $(FLAGS) -fPIC -c -o ../obj/measurement.o measurement.cc

reader.o : reader.cc reader.hh
	$(COMPILER) $(FLAGS) -fPIC -c -o ../obj/reader.o reader.cc

experiment_definition.o : experiment_definition.cc experiment_definition.hh
	$(COMPILER) $(FLAGS) -fPIC -c -o ../obj/experiment_definition.o experiment_definition.cc

#pre_reader.o : pre_reader.cc pre_reader.hh
#	$(COMPILER) $(FLAGS) -fPIC -c -o pre_reader.o pre_reader.cc

trace_algorithms.o : trace_algorithms.cc trace_algorithms.hh
	$(COMPILER) $(FLAGS) -fPIC -c -o ../obj/trace_algorithms.o trace_algorithms.cc

event.o : event.cc event.hh
	$(COMPILER) $(FLAGS) -fPIC -c -o ../obj/event.o event.cc

clean :
	rm ../obj/*.o
	rm ../lib/libpixie.so

#install dirs as appropriate
install :
	cp *.hh ~/.local/include/libpixie/
	cp ../lib/libpixie.so ~/.local/lib/
