#include <cstring>

#include "measurement.hh"
#include "traces.hh"
#include "reader.hh"

namespace PIXIE {
  
  Mask Measurement::mChannelNumber    = Mask(0xF, 0);
  Mask Measurement::mSlotID           = Mask(0xF0, 4);
  Mask Measurement::mCrateID          = Mask(0xF00, 8);
  Mask Measurement::mHeaderLength     = Mask(0x1F000, 12);
  Mask Measurement::mEventLength      = Mask(0x7FFE0000, 17);
  Mask Measurement::mFinishCode       = Mask(0x80000000, 31);
  Mask Measurement::mTimeLow          = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mTimeHigh         = Mask(0xFFFF, 0);
  Mask Measurement::mEventEnergy      = Mask(0xFFFF, 0);
  Mask Measurement::mTraceLength      = Mask(0x7FFF0000, 16);
  Mask Measurement::mTraceOutRange    = Mask(0x80000000, 31);
  
  Mask Measurement::mCFDTime100       = Mask(0x7FFF0000, 16);
  Mask Measurement::mCFDTime250       = Mask(0x3FFF0000, 16);
  Mask Measurement::mCFDTime500       = Mask(0x1FFF0000, 16);
  Mask Measurement::mCFDForce100      = Mask(0x80000000, 31);
  Mask Measurement::mCFDForce250      = Mask(0x80000000, 31);
  Mask Measurement::mCFDTrigSource250 = Mask(0x40000000, 30);
  Mask Measurement::mCFDTrigSource500 = Mask(0xE0000000, 29);

  Mask Measurement::mESumTrailing     = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mESumLeading      = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mESumGap          = Mask(0xFFFFFFFF, 0);
  Mask Measurement::mBaseline         = Mask(0xFFFFFFFF, 0);
  
  Mask Measurement::mQDCSums          = Mask(0xFFFFFFFF, 0);

  CFD Measurement::ProcessCFD(unsigned int data, int frequency) {
    CFD retval = {};
    if (frequency == 100) {
      int CFDTime  = mCFDTime100(data);
      unsigned int CFDForce = mCFDForce100(data);
      retval = {CFDTime, CFDForce};
      return retval;
    }
    else if (frequency == 250) {
      int CFDTime       = mCFDTime250(data);
      unsigned int CFDTrigSource = mCFDTrigSource250(data);
      unsigned int CFDForce      = mCFDForce250(data);
      retval.CFDForce = CFDForce;
      if (CFDForce==0) {                  
        retval.CFDFraction = CFDTime - 16384*static_cast<int>(CFDTrigSource);
      }
      else {
        retval.CFDFraction = 16384;
      }      
    }
    else if (frequency == 500) {
      int CFDTime       = mCFDTime500(data);
      unsigned int CFDTrigSource = mCFDTrigSource500(data);
      if (CFDTrigSource == 7) {
        retval.CFDForce = 1;
        retval.CFDFraction = 40960*4/5;
      }
      else {
        retval.CFDForce = 0;
        retval.CFDFraction = (CFDTime + 8192*(static_cast<int>(CFDTrigSource)-1))*4/5;
      }
    }
    else {
      std::cout << "\n" ANSI_COLOR_RED "Waring! Slot has frequency of neither 100 MHz, 250 MHz, or 500 MHz" ANSI_COLOR_RESET<< std::endl;
    }
    return retval;
  }

  EventTime Measurement::ProcessTime(unsigned long long timestamp, unsigned int cfddat, int frequency) {    
    unsigned long long time = timestamp << 15;
    CFD cfd = ProcessCFD(cfddat, frequency);
    time += cfd.CFDFraction;
    if (frequency == 250) {
      time = time*8/10;
    }
    EventTime retval = {time, cfd.CFDForce};
    return retval;    
  }

  int Measurement::read_header(Reader *reader) {
    uint32_t firstWords[4];
    if (!reader->NSCLDAQ) {
      if (fread(&firstWords, (size_t) sizeof(int)*4, (size_t) 1, reader->file) != 1) {
        return -1;    
      }
    }
    else {
      //some logic here to navigate NSCL data structures and find the next PIXIE fragment?
      if (reader->nsclreader->findNextFragment(reader->file) < 0 ) {
        return -1;
      }
      if (fread(&firstWords, (size_t) sizeof(int)*4, (size_t) 1, reader->file) != 1) {
        return -1;    
      }
      if (reader->nsclreader->presort) { reader->nsclreader->rib_size -= sizeof(int)*4; }
    }
    
    channelNumber = mChannelNumber(firstWords[0]);
    slotID        = mSlotID(firstWords[0]);
    crateID       = mCrateID(firstWords[0]);
    headerLength  = mHeaderLength(firstWords[0]);
    eventLength   = mEventLength(firstWords[0]);
    finishCode    = mFinishCode(firstWords[0]);

    int oldChan = channelNumber;
	     
    uint32_t timestampLow  = mTimeLow(firstWords[1]);
    uint32_t timestampHigh = mTimeHigh(firstWords[2]);
   
    uint64_t timestamp=static_cast<uint64_t>(timestampHigh);
    timestamp=timestamp<<32;
    timestamp=timestamp+timestampLow;

    if (reader->definition.GetChannel(crateID, slotID, channelNumber)) {
      EventTime time = ProcessTime(timestamp, firstWords[2], reader->definition.GetSlot(crateID, slotID)->freq);
      eventTime = time.time;
      CFDForce = time.CFDForce;            
    }
    else {
      std::cout << "\nWarning! CrateID.SlotID.ChannelNumber " << crateID << "." << slotID << "." << channelNumber << " not found" << std::endl;
      eventTime = timestamp<<15;
      eventEnergy=mEventEnergy(firstWords[3]);
      traceLength=mTraceLength(firstWords[3]);
      outOfRange=mTraceOutRange(firstWords[3]);
      print();
    }
    
    eventEnergy=mEventEnergy(firstWords[3]);
    traceLength=mTraceLength(firstWords[3]);
    outOfRange=mTraceOutRange(firstWords[3]);

    return 0;
  }  
  int Measurement::read_full(Reader *reader, uint16_t *outTrace) {
    nTraceMeas = 0;

    if (headerLength == 4) {;}
    else {
      //read the rest of the header
      uint32_t otherWords[headerLength-4];
      if (fread(&otherWords, (size_t) 4, (size_t) headerLength-4, reader->file) != (size_t) headerLength-4) {
        return -1;
      }
      if (reader->NSCLDAQ) { if (reader->nsclreader->presort) { reader->nsclreader->rib_size -= 4*(headerLength-4); } }
      
      //Read the rest of the header
      if (headerLength==8) {//Raw energy sums
        ESumTrailing = mESumTrailing(otherWords[0]);
        ESumLeading  = mESumLeading(otherWords[1]);
        ESumGap      = mESumGap(otherWords[2]);
        baseline     = mBaseline(otherWords[3]);
      }
      else if (headerLength==12) {//QDCSums
        for (int i=0; i<8; i++) {
          QDCSums[i]=mQDCSums(otherWords[i]);
        }
      }
      else if (headerLength==16) { //Both
        ESumTrailing = mESumTrailing(otherWords[0]);
        ESumLeading  = mESumLeading(otherWords[1]);
        ESumGap      = mESumGap(otherWords[2]);
        baseline     = mBaseline(otherWords[3]);
        for (int i=0; i<8; i++) {
          QDCSums[i]=mQDCSums(otherWords[4+i]);
        }
      }
    }

    
    //skip or proces the trace if recorded
    auto channel = reader->definition.GetChannel(crateID, slotID, channelNumber);
    //no trace, do nothing
    if ((eventLength - headerLength) == 0) {;}
    //skip trace if not needed
    else if (!channel) {
      if (fseek(reader->file, (eventLength-headerLength)*4, SEEK_CUR)) {
        return -1;
      }
      if (reader->NSCLDAQ) { if (reader->nsclreader->presort) { reader->nsclreader->rib_size -= 4*(eventLength-headerLength); } }
    }
    else if (outTrace==NULL && !channel->traces){
      if (fseek(reader->file, (eventLength-headerLength)*4, SEEK_CUR)) {
        return -1;
      }
      if (reader->NSCLDAQ) { if (reader->nsclreader->presort) { reader->nsclreader->rib_size -= 4*(eventLength-headerLength); } }
    }
    //Read trace
    else {
      //uint16_t trace[traceLength]; // = {0};
      if (traceLength > MAX_TRACE_LENGTH) { std::cout << "WARNING: traceLength > MAX_TRACE_LENGTH" << std::endl; }
      
      if (fread(&(this->trace[0]), (size_t) 2, (size_t) traceLength, reader->file) != (size_t) traceLength) {
        return -1;
      }
      if (reader->NSCLDAQ) { if (reader->nsclreader->presort) { reader->nsclreader->rib_size -= 2*traceLength; } }
      
      if (outTrace!=NULL){
        memcpy(outTrace, &trace,traceLength*sizeof(uint16_t));
      }
      if (channel->traces) {
        PIXIE::Trace::Algorithm *tracealg = channel->alg;
        if (!tracealg || !tracealg->is_loaded) {
          //no trace algorigthm, should never happen
        }
        else {
          auto tmeas = tracealg->Process(trace, traceLength);
          good_trace = tracealg->good_trace;
          for (int i=0; i<tmeas.size(); ++i) {
            trace_meas[nTraceMeas] = tmeas[i];
            ++nTraceMeas;
          }
        }
      }
    }

    return 0;     
  }  
    
  int Measurement::print() const
  {
    std::cout << "               Crate ID: " << crateID << std::endl;
    std::cout << "                Slot ID: " << slotID << std::endl;
    std::cout << "         Channel Number: " << channelNumber << std::endl;
    std::cout << "            Finish Code: " << finishCode << std::endl;
    std::cout << "             Event Time: " << eventTime << std::endl;
    std::cout << "          Event RelTime: " << eventRelTime << std::endl;    
    std::cout << "              CFD Force: " << CFDForce << std::endl;
    std::cout << "           Event Energy: " << eventEnergy << std::endl;
    std::cout << "           Out of Range: " << outOfRange << std::endl;
    
    if (headerLength==8 || headerLength == 16) {
      std::cout << "   Energy Sum (trailing): " << ESumTrailing << std::endl;
      std::cout << "    Energy Sum (leading): " << ESumLeading << std::endl;
      std::cout << "        Energy Sum (gap): " << ESumGap << std::endl;
      std::cout << "                Baseline: " << baseline << std::endl;
    }
    
    if (headerLength==12 || headerLength == 16) {
      std::cout << "                QDCSum0: " << QDCSums[0] << std::endl;
      std::cout << "                QDCSum1: " << QDCSums[1] << std::endl;
      std::cout << "                QDCSum2: " << QDCSums[2] << std::endl;
      std::cout << "                QDCSum3: " << QDCSums[3] << std::endl;
      std::cout << "                QDCSum4: " << QDCSums[4] << std::endl;
      std::cout << "                QDCSum5: " << QDCSums[5] << std::endl;
      std::cout << "                QDCSum6: " << QDCSums[6] << std::endl;
      std::cout << "                QDCSum7: " << QDCSums[7] << std::endl;
    }

    for (int i=0; i<nTraceMeas; ++i) {
      std::cout << "             TraceMeas"<< i<<": " << trace_meas[i].datum << std::endl;
    }
    return(0);
  }  
}
