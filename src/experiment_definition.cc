/* libpixie experimental definition implementation */

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "experiment_definition.hh"
#include "trace_algorithms.hh"

namespace PIXIE
{
  int Experiment_Definition::open(const std::string &path) {
    if (this->file) {
      return (-1); //file has already been opened
    }

    if (!(this->file = fopen(path.c_str(), "r"))) {
      std::cout << "Warning! " << path << " not opened successfully" << std::endl;              
      return (-2);
    }
    
    return (0);
  }
  
  int Experiment_Definition::read() {
    if (!this->file) {
      return (-1);
    }

    std::stringstream ss;
    char cline[2048];

    int progress = 0;

    while(std::fgets(cline, sizeof cline, this->file)!=NULL) {
      std::string line (cline);
      switch (line[0]){
      case ' ':
      case '\t':
      case '\n':
      case '#':
        break;
      case 'D':
      case 'T':
        {
          ss.clear();
          ss.str(line);

          char flag;
          int crateID;
          int slotID;
          int channelNumber;
          int frequency;
          
          int eraw;
          int qdcs;
          int traces;

          int extwind; //does this channel extend the event build window?

          std::string alg_name;
          std::string alg_file;
          int alg_index = -1;
          ss >> flag;
          ss >> crateID;
          ss >> slotID;
          ss >> channelNumber;
          ss >> frequency;
          if (ss >> eraw) { }
          else { eraw = false; }
          if (ss >> qdcs) { }
          else { qdcs = false; }
          if (ss >> extwind) {  }
          else { extwind = false; }
          if (ss >> traces) { }
          else { traces = false; }

          if (ss >> alg_name) { }
          else { alg_name = ""; }
          if (ss >> alg_file) { }
          else { alg_file = ""; }
          if (ss >> alg_index) { }
          else { alg_index = -1; }          
          
          this->AddCrate(crateID);
          this->AddSlot(crateID, slotID, frequency);
          
          std::string name = "c"+std::to_string(crateID)+"s"+std::to_string(slotID)+"ch"+std::to_string(channelNumber);
          if (flag=='D'){
            if (this->AddChannel(crateID, slotID, channelNumber, eraw, qdcs, extwind, traces, alg_name, alg_file, alg_index,false)!=0) {
              std::cout << "Caution: channel " << crateID << "." << slotID << "." << channelNumber << " defined twice, second definition ignored" << std::endl;
              break;
            }	    
          }
          else if (flag=='T'){
            if (this->AddChannel(crateID, slotID, channelNumber, eraw, qdcs, extwind, traces, alg_name, alg_file, alg_index,true)!=0) {
              std::cout << "Caution: channel " << crateID << "." << slotID << "." << channelNumber << " defined twice, second definition ignored" << std::endl;
              break;
            }	    
          }
          break;
        }
      case 'C':
        break;
      }
    }

    //create algorithm objects
    std::cout << "Setting algorithms" << std::endl;
    int nalgs = set_algorithms();
    std::cout << nalgs << " set" << std::endl;
    
    return (0);
  }//read_definition  
  

  Experiment_Definition::Slot * Experiment_Definition::Crate::GetSlot(int slotID) const {
    //const auto slot = this->slotMap.find(slotID);

    if (slotID-2 < 0) { return NULL; }
    else if (slotID-2 >= MAX_SLOTS_PER_CRATE) { return NULL; }
    
    return slotMap[slotID-2];
  }

  int Experiment_Definition::Crate::AddSlot(int slotID, int frequency) {
    slotMap[slotID-2] = new Slot(crateID, slotID);
    slotMap[slotID-2]->freq = frequency;

    return(0);
  }
  
  Experiment_Definition::Channel * Experiment_Definition::Slot::GetChannel(int channelNumber) const {
    if (channelNumber < 0) { return NULL; }
    else if (channelNumber >= MAX_CHANNELS_PER_BOARD) { return NULL; }

    return channelMap[channelNumber];
  }

  int Experiment_Definition::Slot::AddChannel(int channelNumber,
                                              bool eraw,
                                              bool qdcs,
                                              bool extwind,
                                              bool traces,
                                              std::string algName,
                                              std::string algFile,
                                              int algIndex,
                                              bool isTagger) {
    channelMap[channelNumber] = new Channel(crateID,
                                            slotID,
                                            channelNumber,
                                            eraw,
                                            qdcs,
                                            extwind,
                                            traces,
                                            algName,
                                            algFile,
                                            algIndex,
                                            isTagger);

    return(0);
  }
  
  Experiment_Definition::Crate * Experiment_Definition::GetCrate(int crateID) const
  {//Gets crate with ID crateID, if it doesn't exist then returns     
    if (crateID < 0) { return NULL; }
    else if (crateID >= MAX_CRATES) { return NULL; }
    
    return crateMap[crateID];
  }

  Experiment_Definition::Slot * Experiment_Definition::GetSlot(int crateID, int slotID) const
  {//Gets slot with ID slotID in crate with crateID, if it doesn't exist then returns null
    Experiment_Definition::Crate *crate = GetCrate(crateID);
    if (!crate) {
      return nullptr;
    }
    
    const auto slot = crate->GetSlot(slotID);

    return (slot);    
  }

  Experiment_Definition::Channel * Experiment_Definition::GetChannel(int crateID, int slotID, int channelNumber) const
  {//Gets channel in slot with ID slotID in crate with crateID, if it doesn't exist then returns null
    Experiment_Definition::Slot *slot = GetSlot(crateID, slotID);
    if (!slot) {
      return nullptr;
    }
    const auto channel = slot->GetChannel(channelNumber);
    
    return (channel);
  }

  int Experiment_Definition::AddCrate(int crateID)
  {
    if (GetCrate(crateID)){
      return (-1);  //crate already exists
    }

    crateMap[crateID] = new Crate(crateID);

    return (0);
  }  //Add crate

  int Experiment_Definition::AddSlot(int crateID, int slotID, int frequency)
  {
    if (GetSlot(crateID, slotID))
      {
        return (-1);  //slot already exists 
      }

    //slot does not exist in crate
    auto crate = GetCrate(crateID);

    if (!crate)
      {
        //crate does not exist!
        return (-1);
      }

    //crate exists and slot doesn't

    crate->AddSlot(slotID, frequency);

    return (0);
  } //Add slot

  int Experiment_Definition::AddChannel(int crateID,
                                        int slotID,
                                        int channelNumber,
                                        bool eraw,
                                        bool qdcs,
                                        bool extwind,
                                        bool traces,
                                        std::string algName,
                                        std::string algFile,
                                        int algIndex,
                                        bool isTagger)
  {
    if (GetChannel(crateID, slotID, channelNumber))
      {
        return (-1);  //channel already exists
      }

    //channel does not exist
    auto slot = GetSlot(crateID, slotID);
    
    if (!slot) {
      //slot does not exist!
      return (-1);
    }
    //both crate and slot exist
    slot->AddChannel(channelNumber, eraw, qdcs, extwind, traces, algName, algFile, algIndex, isTagger);

    return (0);
  } //Add channel  

  int Experiment_Definition::print(std::ostream &log)
  {
    for (int i=0; i<MAX_CRATES; ++i) {
      if (crateMap[i] != NULL) {
        auto crate = crateMap[i]; //crate_it.second;
        log << "Crate ID " << crate->crateID << std::endl;
        for (int j=0; j<MAX_SLOTS_PER_CRATE; ++j) {
          if (crate->slotMap[j] != NULL) {
            auto slot = crate->slotMap[j];
            log << "  Slot ID " << slot->slotID << "     Frequency " << slot->freq << " MHz" << std::endl;
            for (int k=0; k<MAX_CHANNELS_PER_BOARD; ++k) {
              if (slot->channelMap[k] != NULL) {
                auto channel = slot->channelMap[k];
                if (channel->extwind) { std::cout << "   Coinc window extends"; }
                else { std::cout << "   Coinc window does not extend"; }
                if (channel->qdcs) { std::cout << "   QDCs"; }
                if (channel->eraw) { std::cout << "   Raw Energy Sums"; }
                if (channel->traces) { std::cout << "   Traces, (" << channel->algName << " \"" << channel->algFile << "\", ID="<<channel->algIndex<<")"; }
                log << "    Channel " << channel->channelNumber;
                if (channel->isTagger) {
                  log << "    (Tagger)";
                }
                
                log << std::endl;
              }
            }            
          }
        }
      }
    }
    return 0;
  }

  int Experiment_Definition::print()
  {
    print(std::cout);
    return 0;
  }

  int Experiment_Definition::set_algorithms() {
    int n_chans = 0;

    for (int i=0; i<MAX_CRATES; ++i) {
      if (crateMap[i] != NULL) {
        auto crate = crateMap[i];
        for (int j=0; j<MAX_SLOTS_PER_CRATE; ++j) {
          if (crate->slotMap[j] != NULL) {
            auto slot = crate->slotMap[j];      
            for (int k=0; k<MAX_CHANNELS_PER_BOARD; ++k) {
              if (slot->channelMap[k] != NULL) {
                auto channel = slot->channelMap[k];
                if (channel->traces) {
                  ++n_chans;
                  int retval = PIXIE::setTraceAlg(channel->alg, channel->algName, channel->algFile, channel->algIndex);
                  if (retval < 0) {
                    std::cout << "Trace Algorithm not set properly!" << std::endl;
                  }
                }
              }
            }
          }
        }
      }
    }
    return n_chans;
  }
    
}//PIXIE
