#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"

#include <libpixie/pixie.hh>

int main(int argc, const char **argv) {
  if (argc < 3) { 
    std::cout << "usage ./BasicSort [file list] [output file]" << std::endl;
    return 0;
  }

  float coincWindow = 3000; //in ns
  double ns = 3276.8; //conversion factor
  double dither = 0.0;
  int nCrates = 1;
  
  PIXIE::Reader reader;
  std::string defPath = "experiment.def";
  reader.definition.open(defPath);
  reader.definition.read();
  //reader.definition.print();
  reader.set_coinc(coincWindow);
  int nfiles = reader.loadfiles(argv[1]);
  if (nfiles == 0) {
    exit(1);
  }
  //reader.nscldaq();
    
  TFile file(argv[2], "recreate");
  //ROOT histogram definitions
  // "raw" histograms - on sub events 
  TH2F *raw_e = new TH2F("raw_e", "Raw Energy; Energy; Channel ID", 8192, 0, 16*4096, nCrates*13*16, 0, nCrates*13*16);  
  TH1F *raw_pu = new TH1F("raw_pu", "Raw Pileup; Channel ID; Counts", nCrates*13*16, 0, nCrates*13*16);
  TH1F *raw_or = new TH1F("raw_or", "Raw Out of Range; Channel ID; Counts", nCrates*13*16, 0, nCrates*13*16);
  TH1F *raw_tot = new TH1F("raw_tot", "Raw Total Counts; Channel ID; Counts", nCrates*13*16, 0, nCrates*13*16);
  TH2F *raw_mult = new TH2F("raw_mult", "Raw Subevent Multiplicity; Wall Time (s); Multiplicity", 4096, 0, 4096, 50, 0, 50);
  TH2F *raw_seqtdiff = new TH2F("raw_seqtdiff", "Raw sequential time difference; Time (us); Channel ID", 4000, 0, 400, nCrates*13*16, 0, nCrates*13*16);
  TH1F *raw_tot_seqtdiff = new TH1F("raw_tot_seqtdiff", "Raw sequential time difference; Time (us); Counts", 4000, 0, 400);
  TH2F *rates = new TH2F("rates", "Rates; Wall Time (s); Channel ID", 4096, 0, 4096, nCrates*13*16, 0, nCrates*13*16);  
  TH1F *rates_ep = new TH1F("rates_ep", "Rates; Wall Time (s); Counts/s", 4096, 0, 4096);
  TH1F *rates_sp = new TH1F("rates_sp", "Rates; Wall Time (s); Counts/s", 4096, 0, 4096);
  TH2F *hitpat = new TH2F("hitpat", "Hit pattern; ID1; ID2", nCrates*13*16, 0, nCrates*13*16, nCrates*13*16, 0, nCrates*13*16);
  TH2F *hitpat_db = new TH2F("hitpat_db", "Hit pattern; ID1; ID2", nCrates*13*16, 0, nCrates*13*16, nCrates*13*16, 0, nCrates*13*16);  
  TH2F *traces = new TH2F("traces", "Traces; Bin; Trace Count", 1000, 0, 1000, 1000, 0, 1000);
 
  long long unsigned int last_time[nCrates*13*16] = {0};
  long long unsigned int last_time_tot = 0;

  time_t now;
  time(&now);
  int traceCount = 0;

  reader.start(); //this is for timing and Ctrl-C catching
  while (true) {
    int retval = reader.read();
    if (retval) { 
      std::cout << "Exiting with retval " << retval << std::endl;
      break;
    }

    PIXIE::Event *e = reader.GetEvent();

    int nMeas = e->nMeas;
    raw_mult->Fill(reader.measurements[e->fMeasurements[0]].eventTime/(ns*1e9), nMeas);
    for (int i=0; i<nMeas; ++i) {
      auto &meas = reader.measurements[e->fMeasurements[i]];
      int crate = meas.crateID;
      int chan = meas.channelNumber;
      int mod = meas.slotID-2;
      int tracelength = meas.traceLength;
      double energy = meas.eventEnergy;
      int indx = crate*13*16+mod*16+chan;
      raw_tot->Fill(indx);
      raw_e->Fill(energy, indx);
      if (meas.finishCode) {
        raw_pu->Fill(indx);
      }
      if (meas.outOfRange) {
        raw_or->Fill(indx);
      }
      raw_seqtdiff->Fill((double)(meas.eventTime - last_time[indx])/ns/1000.0, indx);
      raw_tot_seqtdiff->Fill((double)(meas.eventTime - last_time_tot)/ns/1000.0);
      last_time[indx] = meas.eventTime;
      last_time_tot = meas.eventTime;
      rates->Fill(meas.eventTime/(ns*1e9), indx);

      for (int j=i+1; j<nMeas; ++j) {
        auto &meas2 = reader.measurements[e->fMeasurements[j]];
        int chan2 = meas2.channelNumber;
        int mod2 = meas2.slotID-2;
        hitpat->Fill(indx, mod2*16+chan2);
      }
      if (tracelength>0 && traceCount < 1000 && mod==0 && chan==2) {
        traceCount += 1;
        for (int k=0; k<tracelength; ++k) {
          traces->SetBinContent(k+1,traceCount, meas.trace[k]);
        }
      }
    }     

    if (reader.GetEventNum()%10000 == 0) {
      reader.printUpdate();
    }
  }
  reader.stop(); //prints out timing and returns Ctrl-C
  
  file.Write();
  file.Close();

  return 0;
}
