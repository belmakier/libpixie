#include<iostream>
#include<vector>
#include<thread>
#include<chrono>
#include<unordered_map>
#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cerrno>
#include<sstream>
#include<fstream>

#include<pthread.h>

/* EXTERN */
#include "args/args.hxx"

#include "TROOT.h"
#include "TFile.h"
#include "Compression.h"
#include "TTree.h"
#include "TH1.h"

#include "pixie.hh"
#include "traces.hh"
#include "pixie2root.hh"

namespace PIXIE {  
  void *Process(void *thread) {
    PIXIE::Reader *reader            = &(((PixieThread*)thread) -> reader);
    options options                  = ((PixieThread*)thread) -> opt;
    Experiment_Definition definition = ((PixieThread*)thread) -> definition;
    off_t offset                     = ((PixieThread*)thread) -> offset;
    off_t max_offset                 = ((PixieThread*)thread) -> max_offset;
    int threadNum                    = ((PixieThread*)thread) -> threadNum;

    reader -> definition = definition;
    reader -> thread = threadNum;
    
    //reader -> set_algorithm(((PixieThread*)thread) -> tracealg);    

    std::ofstream log(("Thread_"+std::to_string(threadNum)+".log").c_str());
    log << "[ " << threadNum << " ] " << std::endl;
    log << std::flush;

    TFile outFile((options.path_output+"_"+std::to_string(threadNum)).c_str(), "recreate");
    //outFile.SetCompressionAlgorithm(ROOT::kLZ4);
    //outFile.SetCompressionLevel(3);
  
    if (options.verbose==true) {
      log << "Creating ROOT Tree in file: "<< (options.path_output+"_"+std::to_string(threadNum)).c_str() << std::endl;
      log << std::flush;
    } 

    TTree *tree;
    tree = new TTree("RawTree", "RawTree");

    std::unordered_map<const PIXIE::Experiment_Definition::Channel*, PixieTraceEvent*> channel_to_tracedata;
    std::unordered_map<const PIXIE::Experiment_Definition::Channel*, PixieEvent*> channel_to_data;
    std::unordered_map<const PIXIE::Experiment_Definition::Channel*, PixieTagger*> channel_to_tagger;
  
    //////////////////
    // Add branches //
    //////////////////
    for (const auto &crate_it : reader->definition.crateMap) {
      auto crate = crate_it.second;
      for (const auto &slot_it : crate->slotMap) {
        auto slot = slot_it.second;
        for (const auto &channel_it : slot->channelMap) {
          auto channel = channel_it.second;        
          // tree branch prefix
          std::string branchName(std::to_string(crate->crateID) +
                                 "." +
                                 std::to_string(slot->slotID) +
                                 "." +
                                 std::to_string(channel->channelNumber));  //add options to include the name?
          log << "adding branch " << branchName << std::endl;
          log << std::flush;

          //the branch is a tagger:
          if (std::find(reader->definition.taggers.begin(), reader->definition.taggers.end(), channel) != reader->definition.taggers.end()) { 
            PixieTagger* tag = new PixieTagger(); // IIRC legit use of pointer for the sake of Branch
          
            tree -> Branch((branchName+".taggerTime").c_str(), &(tag->taggerTime));
            tree -> Branch((branchName+".taggerValue").c_str(), &(tag->taggerValue));
            tree -> Branch((branchName+".taggerNew").c_str(), &(tag->taggerNew));
          

            channel_to_tagger.insert({channel, tag});
          } else { //regular measurement
            PixieEvent *data = new PixieEvent();          
            tree -> Branch((branchName+".eventTime").c_str(), &(data->eventTime));
            tree -> Branch((branchName+".eventRelTime").c_str(), &(data->eventRelTime));
            tree -> Branch((branchName+".finishCode").c_str(), &(data->finishCode));
            tree -> Branch((branchName+".CFDForce").c_str(), &(data->CFDForce));
            tree -> Branch((branchName+".eventEnergy").c_str(), &(data->eventEnergy));
            tree -> Branch((branchName+".outOfRange").c_str(), &(data->outOfRange));

            //if Raw Energy Sums enabled
            if (options.rawE) {
              if (channel->eraw) {
                tree -> Branch((branchName+".ESumTrailing").c_str(), &(data->ESumTrailing));
                tree -> Branch((branchName+".ESumLeading").c_str(), &(data->ESumLeading));
                tree -> Branch((branchName+".ESumGap").c_str(), &(data->ESumGap));
                tree -> Branch((branchName+".baseline").c_str(), &(data->baseline));
              }
            }

            //if QDCs enabled
            if (options.QDCs) {
              if (channel->qdcs) {
                for (int i=0; i<8; ++i) {
                  tree -> Branch((branchName+".QDCSum"+std::to_string(i)).c_str(), &(data->QDCSums[i]));
                }
              }
            }

            //if traces enabled
            if (options.traces) {            
              if (channel->traces) {
                if (!channel->alg) {
                  std::cout << "Must provide an algorithm to process traces! " << std::endl;
                }
                else {
                  auto trace_meas = channel->alg->Prototype();
                  PixieTraceEvent *tracedata = new PixieTraceEvent(trace_meas.size());
                  for (int i=0; i<trace_meas.size(); ++i) {
                    PIXIE::Trace::Measurement meas = trace_meas[i];
                    tree->Branch((branchName+"."+meas.name).c_str(), &(tracedata->meas[i]));
                  }
                  channel_to_tracedata.insert({channel, tracedata});
                }
              }
            }

            channel_to_data.insert({channel, data});
          }

        } 
      } 
    } 


    // preallocating should speed up
    std::vector<PIXIE::Event> new_events(options.events_per_read);

    log << "opening the listmode data " << std::endl;
    log << std::flush;
    reader -> open(options.listPath);
    if (options.verbose==true) {
      printf("[ %i ] Opened listmode data file " ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET "\n", threadNum, options.listPath.c_str());
      printf("[ %i ] Processing " ANSI_COLOR_BLUE "%d" ANSI_COLOR_RESET " events at a time, with a coincidence window of " ANSI_COLOR_BLUE "%d" ANSI_COLOR_RESET "\n", threadNum, options.events_per_read, options.coincWindow);
      std::cout << std::flush;
    }

    for (auto& map_entry : channel_to_tagger) {
      //*reinterpret_cast<PixieTagger *>(map_entry.second) = {0, 0, 0};
      map_entry.second -> Reset(); // this is how we do, chill in laid back
    }

    reader -> set_offset(offset);  
    // start timing
    //size_t eventsread = 0;
    log << "starting the event loop " << std::endl;
    log << std::flush;
    reader -> start();
    reader -> set_offset(offset);

    ///////////////
    // read loop //
    ///////////////
    while(true) {
      new_events.clear();
      if (options.breakatevent > 0 && reader->eventsread + options.events_per_read > options.breakatevent){
	reader -> read(new_events, options.coincWindow, options.breakatevent - reader->eventsread, max_offset, options.warnings);
      }
      else{
      	reader -> read(new_events, options.coincWindow, options.events_per_read, max_offset, options.warnings);
      }
      reader->eventsread = reader->eventsread + new_events.size();

      if (new_events.size()) {
        for (auto& event : new_events) {
          int mult = 0;
          for (auto &map_entry : channel_to_data) {
            map_entry.second -> Reset(); // bring the beat back
          }
          for (auto &map_entry : channel_to_tracedata) {
            map_entry.second -> Reset(); // bring the beat back
          }
          for (auto &map_entry : channel_to_tagger) {
            // only change this part of the tagger
            (map_entry.second) -> taggerNew = 0;
          }

          // Done setting up, iterate and fill the event
          for (auto &meas : event.fMeasurements) {
            auto *channel = reader->definition.GetChannel(meas.crateID, meas.slotID, meas.channelNumber);
            const auto& detector = channel_to_data.find(channel);
            const auto& tagger = channel_to_tagger.find(channel);

            if (!(detector == channel_to_data.end())) {
              //it's a detector
              (detector->second)->finishCode   = meas.finishCode;
              (detector->second)->eventTime    = meas.eventTime;
              (detector->second)->eventRelTime = meas.eventRelTime; // time relative to the first trigger plus 1 -A
              (detector->second)->CFDForce     = meas.CFDForce;
              (detector->second)->eventEnergy  = meas.eventEnergy;
              (detector->second)->outOfRange   = meas.outOfRange;

              if (options.rawE) {
                if (channel->eraw) {
                  (detector->second)->ESumTrailing   = meas.ESumTrailing;
                  (detector->second)->ESumLeading   = meas.ESumLeading;
                  (detector->second)->ESumGap   = meas.ESumGap;
                  (detector->second)->baseline   = meas.baseline;
                }
              }

              if (options.QDCs) {
                if (channel->qdcs) {
                  for (int i=0;i<8;++i)
                    {
                      (detector->second)->QDCSums[i]   = meas.QDCSums[i];
                    }
                }
              }
            }

            const auto& tracedetector = channel_to_tracedata.find(channel);

            if (!(tracedetector == channel_to_tracedata.end())) {
              if (options.traces) {
                if (channel->traces) {
                  for (int i=0;i<(tracedetector->second)->meas.size(); ++i) {
                    if (i >= meas.trace_meas.size()) {
                      (tracedetector->second)->meas[i] = 0;
                    }
                    else {
                      (tracedetector->second)->meas[i] = meas.trace_meas[i].datum;
                    }
                  }
                }
              }
            }

            mult += 1;

            if (!(tagger == channel_to_tagger.end())) {
              //it's a tagger
              if ( meas.finishCode == 0 ) {
		//Only update for the first tagger in the event
		if((tagger->second)->taggerNew == 0 ) {
		  (tagger->second)->taggerValue = meas.eventEnergy;  //tag values are stored as energy
		  (tagger->second)->taggerTime = meas.eventTime;  //time at which the tagger fired
		}
		(tagger->second)->taggerNew += 1; // ask Tim Gray about this one.
              }
              continue;
            }
            else {
              continue;
            }
          }
        
          if (mult >= options.minMult) {
            tree -> Fill();
          }
        }// event loop
      }

      tree -> Write();
      reader -> printUpdate();

      if (options.breakatevent && reader->eventsread >= options.breakatevent) {
        tree -> Write(tree->GetName(), TObject::kOverwrite);
        break;
      }

      if (reader->eof() || reader->end) {
        if (options.live && options.liveCount<=10) { //if we hit the end of the file for 50 seconds continuously, assume we have stopped acquiring data and quit
          std::cout << " eof? " << options.liveCount << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          options.liveCount=options.liveCount+1;
          continue;
        } else {
          break;
        }
      } else { options.liveCount=0; }

    } //while (true) loop

    outFile.Purge();
    outFile.Close();
    pthread_exit(NULL);
  }
}
