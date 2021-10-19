#include <iostream>
#include <vector>
#include <map>

#include <TH1.h>
#include <TGraph.h>
#include <TFile.h>

#define MAX_CHANNELS_PER_BOARD 16
#define MCA_CHANNELS 8192
#define MAX_BOARDS 26

struct modstatistics_v0
{                                                        
  time_t timeofday;                                      
  double realtime;                                       
  double livetime[MAX_CHANNELS_PER_BOARD];               
  double inputrate[MAX_CHANNELS_PER_BOARD];              
  double outputrate[MAX_CHANNELS_PER_BOARD];             
  float livetime_eff[MAX_CHANNELS_PER_BOARD];            
};

struct modstatistics_v1
{
  time_t timeofday;
  double realtime;
  double livetime[MAX_CHANNELS_PER_BOARD];
  double inputrate[MAX_CHANNELS_PER_BOARD];
  double outputrate[MAX_CHANNELS_PER_BOARD];
  double chanevents[MAX_CHANNELS_PER_BOARD];
  double fastpeaks[MAX_CHANNELS_PER_BOARD];
  float livetime_per[MAX_CHANNELS_PER_BOARD];
  float pileup_per[MAX_CHANNELS_PER_BOARD];     
};

int main(int argc, const char **argv) {
  FILE* scaFile = NULL;

  if (argc < 4) {
    std::cout << "Usage: pixie_scalers scaler_file num_boards out_file.root [version number]" << std::endl;
    std::cout << "Version number -    0    :    before 02/2019 " << std::endl;
    std::cout << "                    1    :     after 02/2019 " << std::endl;
    return -1;
  }
  
  scaFile = fopen(argv[1], "rb");

  int nBoards = atoi(argv[2]);
  long long unsigned nScalers = 0;
  
  //not ideal but shouldn't have to expand this so it's fine (?)
  modstatistics_v0 scalers_v0[MAX_BOARDS];
  modstatistics_v1 scalers_v1[MAX_BOARDS];
  
  int version = 1;
  if (argc>4) { version = atoi(argv[4]); }

  std::cout << "Version " << version << std::endl;
  
  std::map<int, TGraph*> realtime;
  std::map<std::pair<int, int>, TGraph *> livetime;
  std::map<std::pair<int, int>, TGraph *> inputrate;
  std::map<std::pair<int, int>, TGraph *> outputrate;
  std::map<std::pair<int, int>, TGraph *> chanevents;
  std::map<std::pair<int, int>, TGraph *> fastpeaks;
  std::map<std::pair<int, int>, TGraph *> livetime_eff;
  std::map<std::pair<int, int>, TGraph *> livetime_per;
  std::map<std::pair<int, int>, TGraph *> pileup_per;

  for (int i=0; i<nBoards; ++i) {
    realtime[i] = new TGraph();
    for (int j=0; j<MAX_CHANNELS_PER_BOARD; ++j) {
      livetime[{i,j}]= new TGraph();
      inputrate[{i,j}]= new TGraph();
      outputrate[{i,j}]= new TGraph();
      if (version==0) {
        livetime_eff[{i,j}]= new TGraph();
      }
      else if (version==1) {
        chanevents[{i,j}]= new TGraph();
        fastpeaks[{i,j}]= new TGraph();
        livetime_per[{i,j}]= new TGraph();
        pileup_per[{i,j}]= new TGraph();
      }
    }
  }  
  
  if (version==0) {
    while (fread(scalers_v0, sizeof(scalers_v0), 1, scaFile) == 1) {
      for (int i=0; i<nBoards; ++i) {
        realtime[i]->SetPoint(realtime[i]->GetN(), nScalers, scalers_v0[i].realtime);
        for (int j=0; j<MAX_CHANNELS_PER_BOARD; ++j) {
          livetime[{i,j}]->SetPoint(livetime[{i,j}]->GetN(), nScalers, scalers_v0[i].livetime[j]);
          inputrate[{i,j}]->SetPoint(inputrate[{i,j}]->GetN(), nScalers, scalers_v0[i].inputrate[j]);
          outputrate[{i,j}]->SetPoint(outputrate[{i,j}]->GetN(), nScalers, scalers_v0[i].outputrate[j]);
          livetime_eff[{i,j}]->SetPoint(livetime_eff[{i,j}]->GetN(), nScalers, scalers_v0[i].livetime_eff[j]);
        }
      }
      nScalers++;
    }    
  }
  else if (version==1) {
    while (fread(scalers_v1, sizeof(scalers_v1), 1, scaFile) == 1) {
      for (int i=0; i<nBoards; ++i) {
        realtime[i]->SetPoint(realtime[i]->GetN(), nScalers, scalers_v1[i].realtime);
        for (int j=0; j<MAX_CHANNELS_PER_BOARD; ++j) {
          livetime[{i,j}]->SetPoint(livetime[{i,j}]->GetN(), nScalers, scalers_v1[i].livetime[j]);
          inputrate[{i,j}]->SetPoint(inputrate[{i,j}]->GetN(), nScalers, scalers_v1[i].inputrate[j]);
          outputrate[{i,j}]->SetPoint(outputrate[{i,j}]->GetN(), nScalers, scalers_v1[i].outputrate[j]);
          chanevents[{i,j}]->SetPoint(chanevents[{i,j}]->GetN(), nScalers, scalers_v1[i].chanevents[j]);
          fastpeaks[{i,j}]->SetPoint(fastpeaks[{i,j}]->GetN(), nScalers, scalers_v1[i].fastpeaks[j]);
          livetime_per[{i,j}]->SetPoint(livetime_per[{i,j}]->GetN(), nScalers, scalers_v1[i].livetime_per[j]);
          pileup_per[{i,j}]->SetPoint(pileup_per[{i,j}]->GetN(), nScalers, scalers_v1[i].pileup_per[j]);
        }
      }
      nScalers++;
    }    
  }

  std::cout << nScalers << std::endl;

  //make instant graphs
  std::map<std::pair<int, int>, TGraph *> inst_livetime_per;
  std::map<std::pair<int, int>, TGraph *> inst_pileup_per;
  
  for (int i=0; i<nBoards; ++i) {
    for (int j=0; j<MAX_CHANNELS_PER_BOARD; ++j) {
      inst_livetime_per[{i,j}] = new TGraph();
      inst_pileup_per[{i,j}] = new TGraph();
      double last_diff_fastpeak = 0;
      double last_diff_chanev = 0;
      int increment = 10;
      for (int tic=increment; tic<nScalers; tic+=increment) {
        double inst_livetime;
        double inst_pileup;
        double scaler_n;
        
        double last_realtime;
        double now_realtime;
        realtime[i]->GetPoint(tic-increment, scaler_n, last_realtime);
        realtime[i]->GetPoint(tic, scaler_n, now_realtime);
        double diff_realtime = now_realtime - last_realtime;
        
        double last_livetime;
        double now_livetime;
        livetime[{i,j}]->GetPoint(tic-increment, scaler_n, last_livetime);
        livetime[{i,j}]->GetPoint(tic, scaler_n, now_livetime);
        double diff_livetime = now_livetime - last_livetime;

        inst_livetime = diff_livetime/diff_realtime;
        
        inst_livetime_per[{i,j}]->SetPoint(inst_livetime_per[{i,j}]->GetN(), tic, 100.0*inst_livetime);

        double last_chanev;
        double now_chanev;
        double last_fastpeak;
        double now_fastpeak;

        if (version==0) {
          double last_icr;
          double now_icr;
          inputrate[{i,j}]->GetPoint(tic-increment, scaler_n, last_icr);
          inputrate[{i,j}]->GetPoint(tic, scaler_n, now_icr);

          double last_ocr;
          double now_ocr;
          outputrate[{i,j}]->GetPoint(tic-increment, scaler_n, last_ocr);
          outputrate[{i,j}]->GetPoint(tic, scaler_n, now_ocr);

          double inst_fastpeaks = (now_icr*now_livetime - last_icr*last_livetime)/(now_realtime - last_realtime);
          double inst_chanev = (now_ocr*now_realtime - last_ocr*last_realtime)/(now_realtime - last_realtime);

          if (inst_fastpeaks > 0 && inst_chanev > 0 ) {
            inst_pileup = inst_chanev/inst_fastpeaks;
          }
          else {
            inst_pileup = 0;
          }
        }
        else if (version==1) {
          chanevents[{i,j}]->GetPoint(tic-increment, scaler_n, last_chanev);
          chanevents[{i,j}]->GetPoint(tic, scaler_n, now_chanev);

          fastpeaks[{i,j}]->GetPoint(tic-increment, scaler_n, last_fastpeak);
          fastpeaks[{i,j}]->GetPoint(tic, scaler_n, now_fastpeak);

          double diff_chanev = now_chanev-last_chanev;
          double diff_fastpeak = now_fastpeak-last_fastpeak;

          if ( diff_fastpeak != 0 ) {
            if (last_diff_fastpeak == 0 ) {
              diff_chanev += last_diff_chanev;
            }
            inst_pileup = diff_chanev/diff_fastpeak;

            last_diff_fastpeak = 0;
            last_diff_chanev = 0;
          }
          else {
            last_diff_fastpeak += diff_fastpeak;
            last_diff_chanev += diff_chanev;
          }
          
        }

        inst_pileup_per[{i,j}]->SetPoint(inst_pileup_per[{i,j}]->GetN(), tic, 100.0*inst_pileup);

      }
    }
  }
  
  TFile *outFile = new TFile(argv[3], "recreate");
  outFile->cd();
  for (int i=0; i<nBoards; ++i) {
    realtime[i]->Write(("realtime"+std::to_string(i)).c_str());
    for (int j=0; j<MAX_CHANNELS_PER_BOARD; ++j) {
      livetime[{i,j}]->Write(("livetime"+std::to_string(i)+"_"+std::to_string(j)).c_str());
      inputrate[{i,j}]->Write(("inputrate"+std::to_string(i)+"_"+std::to_string(j)).c_str());
      outputrate[{i,j}]->Write(("outputrate"+std::to_string(i)+"_"+std::to_string(j)).c_str());

      if (version==0) {
        livetime_eff[{i,j}]->Write(("livetime_eff"+std::to_string(i)+"_"+std::to_string(j)).c_str());
      }
      else if (version==1) {
        fastpeaks[{i,j}]->Write(("fastpeaks"+std::to_string(i)+"_"+std::to_string(j)).c_str());
        chanevents[{i,j}]->Write(("chanevents"+std::to_string(i)+"_"+std::to_string(j)).c_str());
        livetime_per[{i,j}]->Write(("livetime_per"+std::to_string(i)+"_"+std::to_string(j)).c_str());
        pileup_per[{i,j}]->Write(("pileup_per"+std::to_string(i)+"_"+std::to_string(j)).c_str());
      }

      inst_pileup_per[{i,j}]->Write(("inst_pileup_per"+std::to_string(i)+"_"+std::to_string(j)).c_str());
      inst_livetime_per[{i,j}]->Write(("inst_livetime_per"+std::to_string(i)+"_"+std::to_string(j)).c_str());

    }
  }
  outFile->Close();
  fclose(scaFile);
}
