/* event implementation for libpixie */

#include <vector>
#include <iostream>

#include "experiment_definition.hh"
#include "event.hh"
#include "colors.hh"
#include "reader.hh"

namespace PIXIE
{
  int Event::read(Reader *reader,
                  int coincWindow,
                  bool warnings) {

    nMeas = 0;
    badcfd = 0;
    pileups = 0;
    outofrange = 0;

    Measurement *meas;

    int retval = 0;
    //only do this the first time
    if (reader->nEvents == 0) {
      retval = AddMeasurement(reader);
      if (retval == -1) {
        nMeas -= 1;
        return retval; //end of file return
      }

      meas = &(reader->measurements[fMeasurements[nMeas-1]]);
    }

    else {
      if (reader->measCtr == 0) {
        meas = &(reader->measurements[MAX_MEAS_PER_EVENT * MAX_EVENTS - 1]);
        fMeasurements[0] = MAX_MEAS_PER_EVENT * MAX_EVENTS - 1;
      }
      else {
        meas = &(reader->measurements[reader->measCtr - 1]);
        fMeasurements[0] = reader->measCtr - 1;
      }

      ++nMeas;
    }
                                    
    int lastCrate = meas->crateID;
    int lastSlot = meas->slotID;
    int lastChan = meas->channelNumber;
    
    uint64_t maxTime = meas->eventTime + coincWindow;
    uint64_t triggerTime = meas->eventTime;

    mult = 1;

    int curEvent = 1;
    while (curEvent) {
      //loop for current event

      retval = AddMeasurement(reader);
           
      if (retval == -1) { //end of file
        break;
      }
      Measurement *next_meas = &(reader->measurements[fMeasurements[nMeas-1]]);
     
      //   get event time, check if it's in the coincidence window
      if (next_meas->eventTime<=maxTime) {
        //in current event
        //check for duplicate in same channel
        if (reader->RejectSCPU) {
          if (GetMeasurement(next_meas->crateID, next_meas->slotID, next_meas->channelNumber, reader)) {
            const Measurement *dup_meas = GetMeasurement(next_meas->crateID, next_meas->slotID, next_meas->channelNumber, reader);
	  
            if (warnings) {
              std::cout << ANSI_COLOR_RED << "Warning: Channel " << dup_meas->slotID << ":" << dup_meas->channelNumber << " already fired in this event, perhaps the coincidence window is too large?" ANSI_COLOR_RESET << std::endl;
              std::cout << ANSI_COLOR_RED << dup_meas->eventTime << " vs " << next_meas->eventTime << " :  " << (double)(-dup_meas->eventTime + next_meas->eventTime)/3276.8 << ANSI_COLOR_RESET << std::endl;
            }
            //if it's not a tagger, end this event and start another
            auto *channel = reader->definition.GetChannel(next_meas->crateID, next_meas->slotID, next_meas->channelNumber);
            if(!channel->isTagger){
              curEvent = 0;
              retval = 1; //end of event with same channel pileup
              break;
            }
          }
        }        
        
        mult = mult + 1;
		 
        if (next_meas->eventTime<maxTime-coincWindow) {
          std::cout<< ANSI_COLOR_RED "\nWarning! File is not properly time-sorted" << std::endl;
          std::cout<< "First event: "
                   << lastCrate << "." 
                   << lastSlot << "." 
                   << lastChan << "   " << maxTime - coincWindow << std::endl;
          std::cout<< "Second event: "
                   << next_meas->crateID << "."
                   << next_meas->slotID << "."
                   << next_meas->channelNumber << "   " << next_meas->eventTime << ANSI_COLOR_RESET << std::endl;

        }
                
        lastCrate = next_meas->crateID;
        lastSlot = next_meas->slotID;
        lastChan = next_meas->channelNumber;
        auto *channel = reader->definition.GetChannel(next_meas->crateID, next_meas->slotID, next_meas->channelNumber);
        if (channel->extwind) {
          maxTime = next_meas->eventTime+coincWindow;
        }
        //go to next sub-event

        if (mult == MAX_MEAS_PER_EVENT-1) {
          retval = 2;
          break;
        }
      }
      else {        
        curEvent=0; //breaks current event loop
      }
    }//loop for current event
    
    nMeas -= 1; //not in this event
    
    return retval;
  }

  int Event::print(Reader *reader)
  {
    std::cout << "======= NEW EVENT ======" << std::endl;
    for (int i=0; i<nMeas; ++i) {
      std::cout << "index = " << fMeasurements[i] << std::endl;
      reader->measurements[fMeasurements[i]].print();
    }
    std::cout << pileups << std::endl;
    std::cout << badcfd << std::endl;
    std::cout << outofrange << std::endl;
    
    return (0);
  }


  const Measurement *Event::GetMeasurement(int crateID, int slotID, int channelNumber, Reader *reader) const {
    for (int i=0; i<nMeas-1; ++i) {
      const Measurement *meas = &(reader->measurements[fMeasurements[i]]);
      if (meas->crateID == crateID &&
          meas->slotID == slotID &&
          meas->channelNumber == channelNumber) {
        return meas;
      }
    }
    return (NULL);
  }

  int Event::AddMeasurement(Reader *reader) {
    int retval = AddMeasHeader(reader);
    if (retval == -1) {
      return retval;
    }
      
    retval = AddMeasFull(reader);
    return retval;
  }
  int Event::AddMeasHeader(Reader *reader) {
    fMeasurements[nMeas] = reader->measCtr;
    Measurement *meas = &(reader->measurements[reader->measCtr]);
    int retval = meas->read_header(reader);
      
    ++nMeas;
    ++reader->measCtr;
    if (reader->measCtr == MAX_MEAS_PER_EVENT * MAX_EVENTS) {
      reader->measCtr = 0;
    }
    return retval;
  }
  int Event::AddMeasFull(Reader *reader) {
    Measurement *meas = &(reader->measurements[fMeasurements[nMeas-1]]);
    int retval = meas->read_full(reader);
    //at this point we know that this belongs in the event
    if (meas->finishCode == 1) {
      ++pileups;
    }

    badcfd += meas->CFDForce;
    outofrange += meas->outOfRange;   
      
    return retval;
  }

  const Measurement *Event::GetMeasurement(Reader *reader, int i) const {
    return &(reader->measurements[fMeasurements[i]]);
  }
    
}//PIXIE namespace
                       
