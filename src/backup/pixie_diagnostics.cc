#include <iostream>
#include <vector>
#include <thread>

#include <TROOT.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TH1.h>
#include <TFile.h>

#include <ROOT/TThreadedObject.hxx>
#include <ROOT/TTreeProcessorMT.hxx>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct detCounter {
  std::atomic<unsigned long long> total_events;
  std::atomic<unsigned long long> cfdforces;
  std::atomic<unsigned long long> energy_notime;
  std::atomic<unsigned long long> time_noenergy;
  std::atomic<unsigned long long> pileup_events;
  std::atomic<unsigned long long> out_of_range;
  detCounter() : total_events(0),
                 cfdforces(0),
                 energy_notime(0),
                 time_noenergy(0),
                 pileup_events(0),
                 out_of_range(0) {}
  
};

struct detReader {
  TTreeReaderValue<unsigned int> energy;
  TTreeReaderValue<ULong64_t> time;
  TTreeReaderValue<unsigned int> pileup;
  TTreeReaderValue<unsigned int> cfdforce;
  TTreeReaderValue<unsigned int> outrange;

  detReader(TTreeReader &tr, std::string name) : energy(tr, (name+".eventEnergy").c_str()),
                                                 time(tr, (name+".eventTime").c_str()),
                                                 pileup(tr, (name+".finishCode").c_str()),
                                                 cfdforce(tr, (name+".CFDForce").c_str()),
                                                 outrange(tr, (name+".outOfRange").c_str())                 
  { }

};

struct tagCounter {
  std::atomic<unsigned long long> total_events;
  std::atomic<unsigned long long> val_notime;
  std::atomic<unsigned long long> time_noval;
  tagCounter() : total_events(0),
                 val_notime(0),
                 time_noval(0) {}
};

struct tagReader {
  TTreeReaderValue<unsigned int> value;
  TTreeReaderValue<ULong64_t> time;
  TTreeReaderValue<unsigned int> isnew;

  tagReader(TTreeReader &tr, std::string name) : value(tr, (name+".taggerValue").c_str()),
                                                 time(tr, (name+".taggerTime").c_str()),
                                                 isnew(tr, (name+".taggerNew").c_str())
  { }

};

int main(int argc, char **argv) {

  //ROOT::EnableImplicitMT(std::thread::hardware_concurrency());
  //ROOT::EnableImplicitMT(1);
  int opt;
  long long unsigned maxEvent = 0;
  while ((opt=getopt(argc, argv, "hN:"))!= -1) {
    switch(opt) {
    case 'h': {
      std::cout << "pixie_diagnostics [-h] [-N events] rawtree_file.root" << std::endl;
      return(0);
    }
    case 'N': {
      maxEvent = atoll(optarg);
      break;
    }
    default: {
      break;
    }
    }
  }   

  argc -= optind;
  argv += optind;

  std::string filename(*argv);
  TFile *file = new TFile(filename.c_str());
  TTree *tree = (TTree*)file->Get("RawTree");
  
  TTreeReader reader("RawTree", file);
  
  TObjArray *branches = tree->GetListOfBranches();

  std::map< std::string, std::string> channels;


  int nDetectors = 0;
  int nTaggers = 0;
  
  for (int i=0; i<branches->GetLast(); ++i) {
    TBranch *branch = (TBranch*)branches->At(i);
    std::string sName(branch->GetName());
    std::istringstream is;
    is.str(sName);
    std::string crate;
    std::string slot;
    std::string chan;
    std::string type;
    std::getline(is,crate,'.');
    std::getline(is,slot,'.');
    std::getline(is,chan,'.');
    std::getline(is,type);

    std::string chan_name = crate+"."+slot+"."+chan;
    if (channels.find(chan_name) != channels.end()) {
      continue;
    }

    if (!strncmp(type.c_str(), "event", 5)) {
      channels[chan_name] = "detector";
      nDetectors += 1;
    }
    else if (!strncmp(type.c_str(), "tagger", 6)) {
      channels[chan_name] = "tagger";
      nTaggers +=1 ;
    }
  }

  std::map< std::string, detCounter*> detCounters;
  std::map< std::string, tagCounter*> tagCounters;
 
  for (auto &chan : channels) {
    //std::cout << chan.first << "   " << chan.second << std::endl;
    if (!strncmp(chan.second.c_str(), "detector", 8)) { 
      detCounters[chan.first] = new detCounter();
    }
    if (!strncmp(chan.second.c_str(), "tagger", 6)) { 
      tagCounters[chan.first] = new tagCounter();
    }
  }  

  std::cout << ANSI_COLOR_BLUE << nDetectors << ANSI_COLOR_RESET << " detector";
  if (nDetectors != 1) { std::cout << "s"; }
  std::cout << ", " << ANSI_COLOR_BLUE << nTaggers << ANSI_COLOR_RESET << " tagger";
  if (nTaggers != 1) {std::cout << "s"; }
  std::cout << std::endl;

  //TChain *chain = new TChain("RawTree");
  //chain->Add(filename.c_str());
  //ROOT::TTreeProcessorMT processor(*chain);
  
  std::atomic<unsigned long long>  mults[8];
  std::atomic<unsigned long long>  tag_mults[8];

  for (int i=0; i<8; ++i) {
    mults[i] = 0;
    tag_mults[i] = 0;
  }

  std::atomic<long long unsigned> eventNo(0);
  long long unsigned nEvents = tree->GetEntries();
  std::cout << nEvents << std::endl;
  
  auto sort = [&](TTreeReader &reader) {
    std::cout << "sort called" << std::endl;
    std::map< std::string, detReader*> detReaders;
    std::map< std::string, tagReader*> tagReaders;
    
    for (auto &chan : channels) {
      //std::cout << chan.first << "   " << chan.second << std::endl;
      if (!strncmp(chan.second.c_str(), "detector", 8)) { 
        detReaders[chan.first] = new detReader(reader, chan.first);
      }
      if (!strncmp(chan.second.c_str(), "tagger", 6)) { 
        tagReaders[chan.first] = new tagReader(reader, chan.first);
      }
    }

    std::cout << "entering event loop " << reader.GetEntries(kTRUE) << std::endl;
    std::cout << reader.SetEntry(0) << std::endl;
    std::cout << "actually entering loop" << std::endl;
    while (reader.Next() && (maxEvent == 0 || eventNo < maxEvent) ) {
      ++eventNo;
      if (eventNo % 10000 == 0 ) {
        printf("\r%llu/%llu     [" ANSI_COLOR_GREEN "%4.1f%%" ANSI_COLOR_RESET "]", static_cast<ULong64_t>(eventNo), nEvents, 100.0*static_cast<double>(eventNo)/static_cast<double>(nEvents));
      }
      std::cout << std::flush;
      int dets_fired = 0;
      for (auto &det : detReaders) {
        //for every channel
        unsigned int en = *(det.second->energy);
        unsigned long long t =  *(det.second->time);
        unsigned int pu = *(det.second->pileup);
        unsigned int cfdf = *(det.second->cfdforce);
        unsigned int outrange = *(det.second->outrange);
        
        if (en==0 && t==0 && pu==0 && cfdf==0 && outrange==0) { continue; }

        detCounter *counter = detCounters[det.first];
      
        dets_fired+=1;
        counter->total_events+=1;

        if (pu) { counter->pileup_events += 1; }
        if (outrange) { counter->out_of_range += 1; }
        if (en && !t && !pu) { counter->energy_notime += 1; std::cout << "\nNo time in event " << eventNo << std::endl;}
        if (!en && t && !pu) { counter->time_noenergy += 1; }
        if (cfdf) { counter->cfdforces += 1; }
      }
      if (dets_fired < 9) {
        mults[dets_fired]+=1;
      }

      int tags_fired = 0;
      for (auto &tag : tagReaders) {
        unsigned int val = *(tag.second->value);
        unsigned long long t =  *(tag.second->time);
        unsigned int nt = *(tag.second->isnew);

        if (!nt) { continue; }

        tagCounter *counter = tagCounters[tag.first];

        tags_fired+=1;
        counter->total_events += 1;

        if (val && !t) { counter->val_notime +=1; }
        if (!val && t) { counter->time_noval +=1; }
      }
      if (tags_fired < 9) {
        tag_mults[tags_fired]+=1;
      }
    }
  };

  std::cout << "sorting..." << std::endl;
  //processor.Process(sort);
  sort(reader);

  std::cout << std::endl;
  if (nDetectors) {
    std::cout << std::endl;
    std::cout << "Detectors: " << std::endl;
    for (int i=0; i<8; ++i) {
      std::cout << "Mult " << i << "  " << ANSI_COLOR_BLUE << mults[i] << ANSI_COLOR_RESET << std::endl;
    }

    printf("     Det           Total              Pile Up               E no T               T no E           out of range              bad CFD\n");
    for (auto &det : detCounters) {
      ULong64_t te = static_cast<ULong64_t>(det.second->total_events);
      ULong64_t pu = static_cast<ULong64_t>(det.second->pileup_events);
      ULong64_t ent = static_cast<ULong64_t>(det.second->energy_notime);
      ULong64_t tne = static_cast<ULong64_t>(det.second->time_noenergy);
      ULong64_t cfdf = static_cast<ULong64_t>(det.second->cfdforces);
      ULong64_t outrange = static_cast<ULong64_t>(det.second->out_of_range);
      
      printf(ANSI_COLOR_CYAN "%8s" ANSI_COLOR_RESET ":  " ANSI_COLOR_BLUE " %12llu "
             "%12llu  " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]   " ANSI_COLOR_BLUE
             "%8llu  " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]   " ANSI_COLOR_BLUE
             "%8llu  " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]   " ANSI_COLOR_BLUE
             "%8llu  " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]   " ANSI_COLOR_BLUE
             "%12llu  " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]\n",
             det.first.c_str(),
             te,  
             pu, 100.*((double)pu/te),
             ent, 100.*((double)ent/te),
             tne, 100.*((double)tne/te),
             outrange, 100.*((double)outrange/te),
             cfdf, 100.*((double)cfdf/te));
    }
  }
  
  if (nTaggers) {
    std::cout << std::endl;
    std::cout << "Taggers: " << std::endl;
    for (int i=0; i<8; ++i) {
      std::cout << "Mult " << i << "  " << ANSI_COLOR_BLUE << tag_mults[i] << ANSI_COLOR_RESET << std::endl;
    }

    printf("     Tag           Total               V no T                T no V\n");
    for (auto &tag : tagCounters) {
      ULong64_t te = static_cast<ULong64_t>(tag.second->total_events);
      ULong64_t vnt = static_cast<ULong64_t>(tag.second->val_notime);
      ULong64_t tnv = static_cast<ULong64_t>(tag.second->time_noval);
      printf(ANSI_COLOR_CYAN "%8s" ANSI_COLOR_RESET ":    " ANSI_COLOR_BLUE "%12llu    "
             "%12llu " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]   " ANSI_COLOR_BLUE
             "%12llu " ANSI_COLOR_RESET "[" ANSI_COLOR_GREEN "%5.2f%%" ANSI_COLOR_RESET "]\n", tag.first.c_str(),
             te, 
             vnt, 100.*((double)vnt/te),
             tnv, 100.*((double)tnv/te));
    }
  }

}
