/* libpixie reader */

#include <sstream>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>

#include "experiment_definition.hh"
#include "reader.hh"
#include "colors.hh"

namespace PIXIE
{
  Reader::Reader()
    : file(nullptr)
  {
    this->pileups      = 0;

    this->sameChanPU   = 0;
    
    this->mults[0]     = 0;
    this->mults[1]     = 0;
    this->mults[2]     = 0;
    this->mults[3]     = 0;

    this->badcfd       = 0;
    this->outofrange   = 0;
    this->subevents    = 0;
    this->nEvents      = 0;

    this->liveSort     = 0;

    this->fileLength   = 0;
    this->fileSize = 0;
    
    this->end          = false;

    this->max_offset   = -1;
    this->start_offset = 0;

    this->thread       = 0;

    this->eventCtr = 0;
    this->measCtr = 0;

    this->warnings = false;
    this->RejectSCPU = false;

    this->NSCLDAQ = false;
    measurements.reserve(MAX_EVENTS*MAX_MEAS_PER_EVENT);
    for (int i=0; i<MAX_EVENTS*MAX_MEAS_PER_EVENT; ++i) {
      measurements.emplace_back();
    }
  }
  
  Reader::~Reader() {}

  off_t Reader::offset() const {
    return (ftello(this->file));
  }

  off_t Reader::set_offset(off_t s_offset) {
    fseek(this->file, s_offset, SEEK_SET);
    this->start_offset = s_offset;
    return s_offset;
  }

  off_t Reader::update_filesize() {
    struct stat stat;
    if (fstat(fileno(this->file), &stat) != 0) {
      std::cout << "Couldn't stat" << std::endl;
    }

    this->fileLength = stat.st_size;
    return(this->fileLength);    
  }


  bool Reader::check_pos() {
    return(this->offset() >= this->update_filesize());
  }    
  
  bool Reader::eof() {    
    assert(this->file);

    if (this->liveSort) {
      if (this->offset()>this->update_filesize()) {
        std::cout<< this->offset() << " " << this->fileLength << std::endl;;
      }
    }
    assert(this->offset() <= this->fileLength);

    return(this->offset() == this->fileLength || std::feof(this->file) );
  }

  void Reader::printUpdate() {
    time_t now;
    time(&now);
    float perc_complete;    
    float size_to_sort = 1;

    if (now==starttime) { return; }
    
    perc_complete = 100.0*(float)((float)ftell(this->file))/fileSize;
    printf("\r[ %i ] Pixie Processing [" ANSI_COLOR_GREEN "%.1f%%" ANSI_COLOR_RESET "], elapsed time " ANSI_COLOR_GREEN "%ld" ANSI_COLOR_RESET " s, " ANSI_COLOR_GREEN "%llu" ANSI_COLOR_RESET " events per s",this->thread, (perc_complete), now-starttime, this->nEvents/(now-starttime));
    std::cout<<std::flush;
  }

  void Reader::printSummary() {
    printf("\n");
    printf("Average event length:     " ANSI_COLOR_YELLOW "%6.5f us\n" ANSI_COLOR_RESET, av_evt_length);
    printf("Total sub-events:     " ANSI_COLOR_YELLOW "%15lld\n" ANSI_COLOR_RESET, subevents);
    printf("Bad CFD sub-events:   " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", badcfd, 100*(1-(double)badcfd/(double)subevents));
    printf("Pileups:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", pileups, 100*(1-(double)pileups/(double)subevents));
    if (RejectSCPU) {
      printf("Same channel pileups: " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", sameChanPU, 100*(1-(double)sameChanPU/(double)subevents));
    }
    printf("Out of range:         " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "good\n", outofrange, 100*(1-(double)outofrange/(double)subevents));
    printf("\n");
    printf("Singles:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", mults[0], 100*(double)mults[0]/(double)nEvents);
    printf("Doubles:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", mults[1], 100*(double)mults[1]/(double)nEvents);
    printf("Triples:              " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", mults[2], 100*(double)mults[2]/(double)nEvents);
    printf("Quadruples:           " ANSI_COLOR_YELLOW "%15lld" ANSI_COLOR_RESET ",     " ANSI_COLOR_GREEN "%5.1f%% " ANSI_COLOR_RESET "\n", mults[3], 100*(double)mults[3]/(double)nEvents);
  }
    
  int Reader::open(const std::string &path) {    
    if (this->file) {
      return (-1); //file has already been opened
    }

    if (!(this->file = fopen(path.c_str(), "rb"))) {      
      return (-1);
    }

    fseek(this->file, 0L, SEEK_END);
    fileSize = ftell(this->file);
    rewind(this->file);
    
    if (this->liveSort) {fseek(this->file, 0, SEEK_END); this->end = true; usleep(1000000);}

    return (0);
  }

  int Reader::read() {
    if (eventCtr == MAX_EVENTS) {
      eventCtr = 0;
    }
    
    this->end = false;

    Event *event = &(events[eventCtr]); //pointer to event
    int retval = event->read(this, coincWindow, warnings);

    if (retval == 0) {}  //successful read
    else if (retval == -1) { //end of file 
      this->end = true;
      return -1;
    }
    else if (retval == 1) { //same channel pileup
      this->sameChanPU += 1;                    
    }
    else if (retval == 2) { //MAX_MEAS_PER_EVENT exceeded
      std::cerr << "Severe warning! MAX_MEAS_PER_EVENT exceeded" << std::endl;
    }
        
    //increment counters
    this->pileups += event->pileups;
    this->badcfd += event->badcfd;
    this->subevents += event->nMeas;
    this->nEvents += 1;
    this->outofrange += event->outofrange;
    if (event->mult < 5) {
      this->mults[event->mult-1] += 1;
    }

    ++eventCtr;

    return 0;
  }//Reader::read

  int Reader::dump_traces(int crate, int slot, int chan, std::string outPath, int maxTraces, int append, std::string traceName) {
    fpos_t pos;
    fgetpos(this->file, &pos);          

    int retval=0;

    //Determine the length of traces for the given channel/if there are traces for the given channel
    int traceLen = 0;
    while (traceLen == 0){
      PIXIE::Measurement meas;
      int retval = meas.read_header(this);
      retval = meas.read_full(this, NULL);
      if (retval<0){break;}
      else if (!meas.good_trace || crate!=meas.crateID || slot!=meas.slotID || chan!=meas.channelNumber){;}
      else {
        traceLen = meas.traceLength;        
      }
    }
    if(traceLen == 0){printf("No traces for given channel\n");exit(1);}

    fsetpos(this->file, &pos);  //we were never even here
    //Draw traces
    double superTrace[traceLen];//={0};

    double x[traceLen];//={0};    
    int nTraces=0;
    printf("Dumping %d trace(s) from slot: %d chan: %d\n",maxTraces, slot, chan);
    while (nTraces<maxTraces){
      uint16_t trace[traceLen];//={0};
      PIXIE::Measurement meas;
      int retval = meas.read_header(this);
      retval = meas.read_full(this, trace);
      if (retval<0){break;}
      else if (!meas.good_trace || crate!=meas.crateID || slot!=meas.slotID || chan!=meas.channelNumber){;}
      else {
        traceLen = meas.traceLength;
        this->definition.GetChannel(crate, slot, chan)->alg->dumpTrace(&trace[0], traceLen, nTraces, outPath, append, traceName);        
        nTraces+=1;
        for(int i=0;i<traceLen;i++){
          superTrace[i] = (double) ((superTrace[i]*(nTraces-1) +  trace[i])/nTraces);
        }
      }
    }
    uint16_t tempTrace[traceLen];//={0};

    for(int i=0;i<traceLen;i++){
      tempTrace[i] = (uint16_t) superTrace[i];
    }
    if (nTraces==0){
      printf("%s!!!Warning!!!  No traces saved%s\n",ANSI_COLOR_RED,ANSI_COLOR_RESET);
      retval = -1;
    }
    else {
      if (nTraces<maxTraces){
        printf("%s!!!Warning!!!  Only %d traces saved%s\n",ANSI_COLOR_RED,nTraces,ANSI_COLOR_RESET);
      }
      retval=1;
      this->definition.GetChannel(crate, slot, chan)->alg->dumpTrace(&tempTrace[0], traceLen, nTraces, outPath, append, traceName+"_SuperTrace");
    }

    fsetpos(this->file, &pos);  //we were never even here
    return 1;
    
  }
  
  int Reader::set_algorithm(PIXIE::Trace::Algorithm *&alg) {
    this->tracealg = alg;
    return 0;
  }

  float Reader::dither;
  float Reader::Dither() { dither += 0.1; if (dither >= 1.0) { dither = 0.0; } return dither; }
}//PIXIE
