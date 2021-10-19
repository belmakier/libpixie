#ifndef LIBPIXIE_MEASUREMENT_H
#define LIBPIXIE_MEASUREMENT_H

#include <iostream>
#include <string>
#include <vector>

#include "experiment_definition.hh"
#include "traces.hh"
#include "colors.hh"
#include "definitions.hh"

namespace PIXIE {
  class Reader;
  
  struct Mask {
    unsigned int fBits;
    unsigned int fShift;

    Mask(const unsigned int bits, unsigned int shift) : fBits(bits), fShift(shift) {}
    const unsigned int operator() (const unsigned int &data) const {
      return (data&fBits) >> fShift;
    }
  };
  
  struct CFD {
    int CFDFraction;
    unsigned int CFDForce;
  };

  struct EventTime {
    unsigned long long time;
    unsigned int CFDForce;
  };
  
  class Measurement {
  public:
    //regular things
    uint32_t headerLength;
    uint32_t eventLength;
    uint32_t traceLength;
    uint32_t crateID;
    uint32_t slotID;
    uint32_t channelNumber;
    uint32_t finishCode;
    uint64_t eventTime;
    uint32_t eventRelTime;    
    uint32_t CFDForce;
    uint32_t eventEnergy;
    uint32_t outOfRange;

    //energy sums
    uint32_t ESumTrailing;
    uint32_t ESumLeading;
    uint32_t ESumGap;
    uint32_t baseline;

    //QDC sums
    uint32_t QDCSums[8];

    //Trace measurements
    PIXIE::Trace::Measurement trace_meas[MAX_TRACE_MEAS];
    uint16_t trace[MAX_TRACE_LENGTH];
    int nTraceMeas;
    bool good_trace;
    
    static Mask mChannelNumber;
    static Mask mSlotID;
    static Mask mCrateID;
    static Mask mHeaderLength;
    static Mask mEventLength;
    static Mask mFinishCode;
    static Mask mTimeLow;
    static Mask mTimeHigh;
    static Mask mEventEnergy;
    static Mask mTraceLength;
    static Mask mTraceOutRange;
    
    static Mask mCFDTime100;
    static Mask mCFDTime250;
    static Mask mCFDTime500;
    static Mask mCFDForce100;
    static Mask mCFDForce250;
    static Mask mCFDTrigSource250;
    static Mask mCFDTrigSource500;

    static Mask mESumTrailing;
    static Mask mESumLeading;
    static Mask mESumGap;
    static Mask mBaseline;
    
    static Mask mQDCSums;

  public:
    Measurement() :
      headerLength(0),
      eventLength(0),
      traceLength(0),      
      crateID(0),
      slotID(0),
      channelNumber(0),
      finishCode(0),
      eventTime(0),
      eventRelTime(0),
      CFDForce(0),
      eventEnergy(0),
      outOfRange(0),
      ESumTrailing(0),
      ESumLeading(0),
      ESumGap(0),
      baseline(0),
      good_trace(false),
      nTraceMeas(0) {      
      std::fill(QDCSums, QDCSums + 8, 0);
    }

    int print() const;
    int read_header(Reader *reader);
    int read_full(Reader *reader, uint16_t *outTrace=NULL);
    int getTrace(FILE *fpr,  PIXIE::Trace::Algorithm *tracealg, uint16_t* trace);
   
    CFD ProcessCFD(unsigned int data, int frequency);
    EventTime ProcessTime(unsigned long long timestamp, unsigned int cfd, int frequency);
  };
}

#endif
