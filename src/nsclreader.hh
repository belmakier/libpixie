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
  class NSCLReader {
  public:
    uint32_t ri_size;
    uint32_t ri_type;
    
    int rib_size; //this tells us the size of the event => how many PIXIE fragments
    
    int readRingItemHeader(FILE *file);
    int readRingItemBodyHeader(FILE *file);
    int readRingItemBody(FILE *file);
    
    int readNextFragment(FILE *file);

    int findNextFragment(FILE *file);
  public:
    NSCLReader() : ri_size(0), ri_type(0), rib_size(0) {};
    ~NSCLReader() {};
  };
}

#endif //LIBPIXIE_NSCLREADER_H
