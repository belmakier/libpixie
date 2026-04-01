GCC=g++
CFLAGS=-I`root-config --incdir` -I$(HOME)/.local/include/ -std=c++17 -fPIC
ROOTFLAGS=`root-config --libs` -L$(HOME)/.local/lib/ -lpixie

all : BasicSort

BasicSort: BasicSort.cc
	$(GCC) $(CFLAGS) -o BasicSort BasicSort.cc $(ROOTFLAGS)
