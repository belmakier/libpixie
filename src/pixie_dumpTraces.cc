/*
  pixie2root: converts an XIA Pixie-16 (time-sorted) listmode data file into a ROOT Tree
  Branches are named as crate.slot.channel, filled with 0 if nothing fired
  Takes a coincidence window as an input
*/

#include<iostream>
#include<vector>
#include<unordered_map>
#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cerrno>
#include<sstream>
#include<fstream>

#include<pthread.h>

/* EXTERN */
#include "args/args.hxx"

#include "TROOT.h"
#include "TFile.h"
#include "Compression.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TMultiGraph.h"

#include "pixie.hh"
#include "trace_algorithms.hh"
#include "pixie2root.hh"

static const struct PixieEvent EmptyChannel;

int main(int argc, char ** argv) {
  args::ArgumentParser parser("pixie_trace utility","Timothy Gray <timothy.gray@anu.edu.au");

  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::ValueFlag<ULong64_t> n_events(parser, "1000", "Events to dump", {'N', "n_events"}, 1000);
  args::ValueFlag<UInt_t> crateID(parser, "0", "Crate Number", {'r', "crateID"}, 0);
  args::ValueFlag<UInt_t> slotID(parser, "2", "Slot Number", {'s', "slotID"}, 2);
  args::ValueFlag<UInt_t> chanID(parser, "0", "Channel Number", {'m', "chanID"}, 0);
  args::ValueFlag<std::string> traceN(parser, "Trace", "Output trace name", {'t', "traceName"}, "Trace");

  args::Flag append(parser, "append", "append to file", {'a', "append"}, 1);

  args::Group filegroup(parser, "Required files", args::Group::Validators::All);
  args::ValueFlag<std::string> expdef(filegroup, "exptdef.expt", "Experimental definition file", {'d', "expdef"});
  args::ValueFlag<std::string> listmode(filegroup, "pixie_data.evt.to", "Listmode data file", {'i', "listmode"});
  args::ValueFlag<std::string> rootfile(filegroup, "test.root", "Output ROOT file", {'o', "rootfile"});
  
  //args::Group tracegroup(parser, "Trace Handling", args::Group::Validators::AllOrNone);
  //args::Group tracealgtype(tracegroup, "Trace Algorithm", args::Group::Validators::Xor);

  //get all the args objects from trace_algorithms.hh
  //auto tracealgs = PIXIE::makeOptions(tracealgtype);
    
  try { parser.ParseCLI(argc, argv); }
  catch (args::Help) {
    std::cout << parser;
    return 0;
  }
  catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }
  catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 2;
  }
  
  int nEvents          = args::get(n_events);
  int app              = args::get(append);
  int crate            = args::get(crateID);
  int slot             = args::get(slotID);
  int chan             = args::get(chanID);
  std::string defPath  = args::get(expdef).c_str();
  std::string lstPath  = args::get(listmode).c_str();
  std::string outPath  = args::get(rootfile).c_str();
  std::string traceName  = args::get(traceN).c_str();

  // Finished parsing command line, start doing.
  // --------------------------------------------------
  printf("Dumping traces from %s%s%s to %s%s%s\n",ANSI_COLOR_RED,lstPath.c_str(),ANSI_COLOR_RESET,ANSI_COLOR_RED,outPath.c_str(),ANSI_COLOR_RESET);

  PIXIE::Experiment_Definition definition;
  int retval = definition.open(defPath);
  definition.read();
  definition.set_algorithms();
  definition.close();

  
  PIXIE::Reader reader;
  reader.thread = 0;
  reader.definition = definition;
  reader.open(lstPath);
  retval = reader.dump_traces(crate, slot, chan, outPath, nEvents, app, traceName);

  printf("Done\n");
} //main
