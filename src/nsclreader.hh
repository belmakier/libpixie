// -*-c++-*-
/* libpixie reader */

#ifndef LIBPIXIE_NSCLREADER_H
#define LIBPIXIE_NSCLREADER_H

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "event.hh"
#include "traces.hh"
#include "definitions.hh"

namespace PIXIE {
  class Reader;
  
  class NSCLReader {
  public:
    bool presort = false;
    
    uint32_t ri_size;
    uint32_t ri_type;
    
    int rib_size; //this tells us the size of the event => how many PIXIE fragments
    
    int readRingItemHeader(Reader *reader);
    int readRingItemBodyHeader(Reader *reader);
    int readRingItemBody(Reader *reader);
    
    int readNextFragment(Reader *reader);

    int findNextFragment(Reader *reader);
  public:
    NSCLReader() : ri_size(0), ri_type(0), rib_size(0), presort(false) {};
    ~NSCLReader() {};
  };
}

#endif //LIBPIXIE_NSCLREADER_H
