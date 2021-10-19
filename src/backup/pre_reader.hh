#ifndef LIBPIXIE_PRE_READER_H
#define LIBPIXIE_PRE_READER_H

#include <string>
#include <vector>
#include <stdio.h>
#include <sys/stat.h>

#include "experiment_definition.hh"

namespace PIXIE {
  class PreReader {
  public:
    int nThreads;
    off_t fileLength;
    bool end;
    std::vector<off_t> offsets;
    FILE *file;
  public:
    PreReader(int threads) : nThreads(threads), file(NULL) {};
    ~PreReader() {};
    int open(const std::string &path);
    int read(size_t breakatevent=0);
    off_t offset() const;
    void print() const;
  };
}
    
#endif
