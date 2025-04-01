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
#include "nsclreader.hh"

namespace PIXIE {
  /*
  enum class ReadType {
    kFRead = 1,
    kBuffRead = 2,
    kMemMapRead = 3,
    kFullBuffRead = 4
  };
  */
  
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
    long long mults[MAX_MEAS_PER_EVENT];

    long long chPileups[MAX_CRATES*MAX_SLOTS_PER_CRATE*MAX_CHANNELS_PER_BOARD];
    long long chBadCFD[MAX_CRATES*MAX_SLOTS_PER_CRATE*MAX_CHANNELS_PER_BOARD];
    long long chTotal[MAX_CRATES*MAX_SLOTS_PER_CRATE*MAX_CHANNELS_PER_BOARD];
    long long chSCPU[MAX_CRATES*MAX_SLOTS_PER_CRATE*MAX_CHANNELS_PER_BOARD];
    long long chOutOfRange[MAX_CRATES*MAX_SLOTS_PER_CRATE*MAX_CHANNELS_PER_BOARD];
    double warning_thresh;

    unsigned long long int first_time;
    unsigned long long int last_time;
    int coincWindow;
    bool warnings;
    bool RejectSCPU;
    bool extendOnZeros;
    
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
    //PIXIE::Measurement measurements[MAX_EVENTS*MAX_MEAS_PER_EVENT];
    //PIXIE::Measurement *measurements[MAX_EVENTS*MAX_MEAS_PER_EVENT];
    std::vector<PIXIE::Measurement> measurements;

    int measCtr;    
    int eventCtr;

    int fd;
    unsigned int off;
    char *buffer;
    unsigned int *pBuff; //pixie buffer
    unsigned int pOff;
    unsigned int pOffMax;

    static float dither;

    bool NSCLDAQ;
    NSCLReader *nsclreader;
    
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

    int openfile(const std::string &path);
    int loadbuffer();
    int clearbuffer();
    int closefile();
    int read();
    int dump_traces(int crate, int slot, int chan, std::string outPath, int maxTraces, int append, std::string traceName);

    void start() {
      eventsread = 0;
      time(&starttime);
    }

    void nscldaq() {
      NSCLDAQ = true;
      nsclreader = new NSCLReader();
    }
    
  };
  
} // namespace PIXIE
      

#endif //LIBPIXIE_READER_H
