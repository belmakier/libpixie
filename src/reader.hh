// -*-c++-*-
/* libpixie reader */

#ifndef LIBPIXIE_READER_H
#define LIBPIXIE_READER_H

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "event.hh"
#include "traces.hh"
#include "definitions.hh"

namespace PIXIE {
  class Reader {
  public:
    //bool binary;
    Experiment_Definition definition;
    int liveSort; //start at end of file?
    long long pileups;
    long long badcfd;
    long long subevents;
    long long nEvents;
    long long sameChanPU;
    long long outofrange;
    long long mults[4];

    int coincWindow;
    bool warnings;
    bool RejectSCPU;
    
    FILE *file;
    
    PIXIE::Trace::Algorithm *tracealg;

    off_t fileLength;
    long int fileSize;

    time_t starttime;
    size_t eventsread;

    off_t max_offset;
    off_t start_offset;

    int thread;
    
    bool end;

    PIXIE::Event events[MAX_EVENTS];  //>1 for correlations
    PIXIE::Measurement measurements[MAX_EVENTS*MAX_MEAS_PER_EVENT];

    int measCtr;    
    int eventCtr;

    static float dither;
    
  public:
    Reader();
    ~Reader();

    static float Dither(); 
    
    const Experiment_Definition & experiment_definition() const
    {
      return (definition);
    };

    bool eof();
    off_t offset() const;
    off_t set_offset(off_t s_offset);
    off_t update_filesize();
    bool check_pos();
    void printUpdate();
    void printSummary();
    double av_evt_length = 0.0;
    
    int set_algorithm(PIXIE::Trace::Algorithm *&alg);
    int set_coinc(float window) { //window is in ns
      coincWindow = (int)(window*3276.8);
      return coincWindow;
    }

    int open(const std::string &path);
    int read();
    int dump_traces(int crate, int slot, int chan, std::string outPath, int maxTraces, int append, std::string traceName);

    void start() {
      eventsread = 0;
      time(&starttime);
    }
    
  };
  
} // namespace PIXIE
      

#endif //LIBPIXIE_READER_H
