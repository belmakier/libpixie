#include <iostream>

//CERN ROOT
#include <TFile.h>
#include <TH1.h>


#define MAX_CHANNELS_PER_BOARD 16
#define MCA_CHANNELS 8192

int main(int argc, const char** argv) {

  FILE *mcaFile = NULL;

  if (argc < 3) {
    std::cout << "Usage: pixie_mcas mcafile outfile.root" << std::endl; return -1;
  }
  mcaFile = fopen(argv[1], "rb");
  if (!mcaFile) { std::cout << "MCA file does not exist or could not be opened!" << std::endl; return 0; }
  fseek(mcaFile, 0L, SEEK_END);
  int NUM_BOARDS = ftell(mcaFile)/(MCA_CHANNELS*4*MAX_CHANNELS_PER_BOARD);
  fclose(mcaFile);

  int cumulative = 0;
  
  TH1I *hist[MAX_CHANNELS_PER_BOARD*NUM_BOARDS];

  for ( int i=0; i<MAX_CHANNELS_PER_BOARD*NUM_BOARDS; ++i )
    {
      TString histName;
      TString histTitle;
      histName.Form("ID%d_%d", i/16, i%16);
      histTitle.Form("Module %d, Channel %d", i/MAX_CHANNELS_PER_BOARD, i%MAX_CHANNELS_PER_BOARD);
      hist[i] = new TH1I(histName.Data(), histTitle.Data(), MCA_CHANNELS, 0, MCA_CHANNELS);
    }


  mcaFile = fopen(argv[1], "rb");
  uint32_t pastvalues[MCA_CHANNELS] = {0};
  for ( int i=0; i<MAX_CHANNELS_PER_BOARD*NUM_BOARDS; ++i )
    {
      uint32_t histvalues[MCA_CHANNELS];
      int ret = fread(&histvalues[0], 4, MCA_CHANNELS, mcaFile);
      if (ret != MCA_CHANNELS) { std::cout << "error in reading" << std::endl;}
      std::cout << "Filling " << hist[i]->GetName() << std::endl;      
      for ( int j=0; j<MCA_CHANNELS; ++j )
        {
          if (cumulative > 0) {
            hist[i]->SetBinContent(j+1, histvalues[j]-pastvalues[j]);
            pastvalues[j]=histvalues[j];
          }
          else {
            hist[i]->SetBinContent(j+1, histvalues[j]);
          }
        }
    }

  
  TFile *outFile = new TFile(argv[2], "recreate");
  outFile->cd();

  for ( int i=0; i<MAX_CHANNELS_PER_BOARD*NUM_BOARDS; ++i )
    {
      std::cout << "Writing histogram " << hist[i]->GetName() << std::endl;
      hist[i]->Write();	    	    
    }
  outFile->Close();
  fclose(mcaFile);
}
