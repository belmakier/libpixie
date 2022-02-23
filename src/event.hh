// -*-c++-*-
/* libpixie event */

#ifndef LIBPIXIE_EVENT_H
#define LIBPIXIE_EVENT_H

#include <vector>

#include "experiment_definition.hh"
#include "measurement.hh"
#include "traces.hh"
#include "definitions.hh"

namespace PIXIE {
  class Reader;
  
  class Event {
  public:
    int fMeasurements[MAX_MEAS_PER_EVENT];
    
    int nMeas;

    long long pileups;
    long long badcfd;
    long long outofrange;
    int mult;

  public:
    Event() :  //initialise counters to zero
      pileups(0),
      badcfd(0),
      outofrange(0),
      nMeas(0),
      mult(0)
    { };
    int print(Reader *reader);
    int AddMeasurement(Reader *reader);
    int AddMeasHeader(Reader *reader); 
    int AddMeasFull(Reader *reader); 
    
    int read(Reader *reader,
             int coincWindow,
             bool warnings);
    
    const Measurement *GetMeasurement(int crateID, int slotID, int channelNumber, Reader *reader) const;
    const Measurement *GetMeasurement(Reader *reader, int i) const;
  };
  
} // namespace PIXIE

#endif //LIBPIXIE_EVENT_H
