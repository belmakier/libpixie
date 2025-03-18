#include <iostream>
#include<cstdio>
#include<cstdlib>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

#include "traces.hh"
#include "trace_algorithms.hh"
#include "measurement.hh"
#include "colors.hh"

/* EXTERN*/
#if ROOT_COMPILE == 1
#include "TROOT.h"
#include "TH1.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TFile.h"
#endif

namespace PIXIE {
  int setTraceAlg(PIXIE::Trace::Algorithm *&tracealg, std::string algName, std::string algFile, int algIndex) {
    if (!((algName).compare("trapfilter"))) {
      tracealg = new PIXIE::Trace::Trapezoid();
    }
    else if (!((algName).compare("trapfilterqdcs"))) {
      tracealg = new PIXIE::Trace::TrapezoidQDC();
    }
    else if (!((algName).compare("peaktail"))) {
      tracealg = new PIXIE::Trace::PeakTail();
    }
    else if (!((algName).compare("qdc_maxval"))) {
      tracealg = new PIXIE::Trace::QDCMaxVal();
    }
    else {
      std::cerr << "Warning! Unknown trace algorithm " << algName << std::endl;
      return -1;
    }
    tracealg->Load(algFile.c_str(), algIndex);
    return 0;
  }
  
  namespace Trace {   
    //Trapezoid
    void Trapezoid::Load(const char *filename, int index) {
      FILE *fpr = fopen(filename, "r");
      if (fpr!=NULL){
        char line[200];
        int ind=-1;
        while (fgets(line, 200, fpr)!=0) {
          switch(line[0]){
          case ' ':
          case '\t':
          case '\n':
          case '#':
            break;
          default:
            sscanf(line,"%d %d %d %d %d %f %d %d %f %f",&ind,&fL,&fG,&sL,&sG,&tau,&D,&S,&ffThr,&cfdThr);
            std::cout << ind << "  " << fL << "  " << fG << "  " << sL << "  " << sG << "  " << tau << "  " << D << "  " << S << "  " << ffThr << "  " << cfdThr << std::endl;
          }
          if (ind == index) { break; }
        }
        fclose(fpr);
        is_loaded = true;
      }
      else {
        printf("%sUnable to open file %s, traces will not be processed%s\n",ANSI_COLOR_RED, filename, ANSI_COLOR_RESET);
      }
    }

    std::vector<Measurement> Trapezoid::Prototype() {
      std::vector<Measurement> retval;
      Measurement tcp("TraceTCP", 0);
      Measurement zcp("TraceZCP", 0);
      Measurement cfd("TraceCFD", 0);
      Measurement energy("TraceEnergy", 0);

      retval.push_back(tcp);
      retval.push_back(zcp);
      retval.push_back(cfd);
      retval.push_back(energy);

      return retval;
    }

    std::vector<Measurement> Trapezoid::Process(uint16_t *trace, int length) {
      std::vector<Measurement> retval;
      retval = Trapezoid::TrapFilter(trace, length);
      return retval;
    }

    double Trapezoid::GetBaseline(uint16_t *trace, int length) {
      double mean=0;
      //Baseline - get a better algorithm for this                              
      for (int k=0;k<40;k++) {
        mean += trace[k];
      }
      mean = mean/40;

      return mean;
    }
    std::vector<Measurement> Trapezoid::TrapFilter(uint16_t *trace, const int length,int write) {

      good_trace = false;
      std::vector<Measurement> retval;
      int P;
      double M;
      float BL[length];//={0};
      float CFD[length];//={0};
      float sD[length];//={0};
      float sP[length];//={0};
      float sR[length];//={0};
      float sTrap[length];//={0};
      float fD[length];//={0};
      float fP[length];//={0};
      float fR[length];//={0};
      float fTrap[length];//={0};

      int TCP = -1;
      int ZCP = -1;
      int cfd_frac = -1;
      
      int ffTrig =0, cfdTrig=0;

      //Pole-zero correction                                                    
      if (tau==-1) {
        P=0;M=1;
      }
      else {
        P=1;
        M=1/(std::exp(1/(tau))-1);
      }

      double mean = Trapezoid::GetBaseline(trace, length);

      //Trapezoid filter                                                        
      for (int k=0;k<length;k++) {
        BL[k]=trace[k]-mean;

        sD[k]=BL[k];
        fD[k]=BL[k];

        if (k-sL>=0) { sD[k] -= BL[k-sL]; }
        if (k-fL>=0) { fD[k] -= BL[k-fL]; }

        if (k-(sG+sL)>=0) { sD[k] -= BL[k-(sG+sL)]; }
        if (k-(fG+fL)>=0) { fD[k] -= BL[k-(fG+fL)]; }

        if (k-(sG+2*sL)>=0) { sD[k] += BL[k-(sG+2*sL)]; }
        if (k-(fG+2*fL)>=0) { fD[k] += BL[k-(fG+2*fL)]; }

        sP[k]= sD[k];
        fP[k]= fD[k];

        if (k-1>=0) { sP[k] += sP[k-1]; }
        if (k-1>=0) { fP[k] += fP[k-1]; }

        sR[k] = P*sP[k] + M*sD[k];
        fR[k] = P*fP[k] + M*fD[k];

        sTrap[k] = sR[k]/(M*sL);
        fTrap[k] = fR[k]/(M);

        if (k-1>=0) { sTrap[k] += sTrap[k-1]; }
        if (k-1>=0) { fTrap[k] += fTrap[k-1]; }

        CFD[k] = (1-S/8)*fTrap[k] - fTrap[k-D];
        //Find fast trigger and CFD values                                    
        if (fTrap[k-1]<=ffThr && fTrap[k]>ffThr && ffTrig==0) {
          TCP = k-1;
          ffTrig=1;
        }
        if (CFD[k-1]<=cfdThr && CFD[k]>cfdThr && cfdTrig==0) {
          cfdTrig=1;
        }
        if (CFD[k-1]*CFD[k]<0 && cfdTrig==1 && ffTrig==1 && k>TCP && ZCP==-1) {
          ZCP=k-1;
        }

        if (ZCP==-1) {
          cfd_frac = -1;
        }
        else {
          cfd_frac = (32768*CFD[ZCP]/(CFD[ZCP]-CFD[ZCP+1]));
        }
      }

      retval.emplace_back("TraceTCP", TCP);
      retval.emplace_back("TraceZCP", ZCP);
      retval.emplace_back("TraceCFD", cfd_frac);
      retval.emplace_back("TraceEnergy", sTrap[TCP + sL + sG -1]);
      
      if (TCP > 0) { good_trace = true; }

      if (write == 1){

#if ROOT_COMPILE == 1
        //Draw trap filter      
        TMultiGraph *mgSlow = new TMultiGraph("SlowTrap","SlowTrap");
        TMultiGraph *mgFast = new TMultiGraph("FastTrap","FastTrap");
        TMultiGraph *mgCFD = new TMultiGraph("CFD","CFD");
        
        TGraph *slowTrap = new TGraph();
        TGraph *fastTrap = new TGraph();
        TGraph *cfd = new TGraph();
        
        slowTrap->SetLineColor(kRed);
        fastTrap->SetLineColor(kBlue);
        cfd->SetLineColor(kGreen);
        
        slowTrap->SetLineWidth(2);
        fastTrap->SetLineWidth(2);
        cfd->SetLineWidth(2);

        for(int k =0 ;k<length;k++){
          slowTrap->SetPoint(k,k,sTrap[k]);
          fastTrap->SetPoint(k,k,fTrap[k]);
          cfd->SetPoint(k,k,CFD[k]);
        }

        mgSlow->Add(slowTrap);
        mgFast->Add(fastTrap);
        mgCFD->Add(cfd);

        mgSlow->Write();
        mgFast->Write();
        mgCFD->Write();
#endif
      }
      return retval;
    }
    
    int Trapezoid::dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName){
#if ROOT_COMPILE == 1
      TFile *outFile;
      if (append==1){
        outFile = new TFile(fileName.c_str(), "UPDATE");
      }
      else{
        outFile = new TFile(fileName.c_str(), "RECREATE");
      }
      
      //write trace + anything else to file
      TH1D *HTrace=new TH1D((traceName+std::to_string(n_traces)).c_str(),(traceName+std::to_string(n_traces)).c_str(),traceLen,0,traceLen);
      for(int i=0;i<traceLen;i++){
        HTrace->AddBinContent(i,trace[i]-trace[0]);
      }
      HTrace->Write();
      Trapezoid::TrapFilter(trace, traceLen,1);
      
      //Close File
      outFile->Purge();
      outFile->Close();
#endif
      return 0;
    }
    
    //Trapezoid with QDCS
    void TrapezoidQDC::Load(const char *filename, int index) {
      FILE *fpr = fopen(filename, "r");
      if (fpr!=NULL){
        char line[200];
        int ind=-1;
        while (fgets(line, 200, fpr)!=0) {
          switch(line[0]){
          case ' ':
          case '\t':
          case '\n':
          case '#':
            break;
          default:
            sscanf(line,"%d %d %d %d %d %f %d %d %f %f %f %f %f %f %f %f %f %f %s %s %f %f %f %f",&ind, &fL,&fG,&sL,&sG,&tau,&D,&S,&ffThr,&cfdThr,&QDCWindows[0],&QDCWindows[1],&QDCWindows[2],&QDCWindows[3],&QDCWindows[4],&QDCWindows[5],&QDCWindows[6],&QDCWindows[7], stringFast, stringSlow, &enLo, &enHi, &pidLo, &pidHi);
          }
          if (ind == index) {
            break;
          }
        }
        fclose(fpr); fpr=NULL;
        is_loaded = true;
      }
      else {
        printf("%sUnable to open file %s%s%s, traces will not be processed%s\n",ANSI_COLOR_RED, ANSI_COLOR_YELLOW,filename,ANSI_COLOR_RED, ANSI_COLOR_RESET);
      }
    }

    std::vector<Measurement> TrapezoidQDC::Prototype() {
      std::vector<Measurement> retval;
      Measurement tcp("TraceTCP", 0);
      Measurement zcp("TraceZCP", 0);
      Measurement cfd("TraceCFD", 0);
      Measurement energy("TraceEnergy", 0);
      Measurement QTime("TraceQTime", 0);
      
      retval.push_back(tcp);
      retval.push_back(zcp);
      retval.push_back(cfd);
      retval.push_back(energy);
      retval.push_back(QTime);
      
      for (int i=0; i<8; ++i) {
        Measurement qdc("TraceQDC"+std::to_string(i), 0);
        retval.push_back(qdc);
      }

      Measurement qdcFast("TraceQDCFast", 0);
      retval.push_back(qdcFast);

      Measurement qdcSlow("TraceQDCSlow", 0);
      retval.push_back(qdcSlow);

      Measurement qdcPID("TraceQDCPID", 0);
      retval.push_back(qdcPID);

      Measurement qdcTotal("TraceQDCTot", 0);
      retval.push_back(qdcTotal);

      return retval;
    }
    
    std::vector<Measurement> TrapezoidQDC::Process(uint16_t *trace, int length) {
      std::vector<Measurement> retval;
      float BL[length];//={0};
      double QDCSums[8];//={0};
      int prevQDC=0;
      int currentQDC=0;
      
      double Q=0, QTime=0;
      
      retval = Trapezoid::TrapFilter(trace, length);
      
      double mean = Trapezoid::GetBaseline(trace, length);
      
      for (int k=0;k<length;k++) {
        BL[k]=trace[k]-mean;
	
        //Get QDCSums
        if ((k-prevQDC)>=QDCWindows[currentQDC]) {
          prevQDC+=QDCWindows[currentQDC];
          currentQDC+=1;
        }
        if (currentQDC<8){	  
          QDCSums[currentQDC]+=BL[k];
        }

        //Get QTime
        Q+=BL[k];
        QTime += BL[k]*k;
      }
      QTime = QTime / Q;      

      Measurement qtime("TraceQTime", (int16_t) QTime);
      retval.push_back(qtime);

      double qdcT=0,qdcF=0,qdcS=0,qdcP;
      char *ptr;
      long int slowMask=strtol(stringSlow,&ptr,2), fastMask=strtol(stringFast,&ptr,2);
      for (int i=0; i<8; ++i) {
        Measurement qdc("TraceQDC"+std::to_string(i), round(QDCSums[i]));
        retval.push_back(qdc);
        if(slowMask & (1<<(7-i))){
          qdcS += round(QDCSums[i]);
        }
        if(fastMask & (1<<(7-i))){
          qdcF += round(QDCSums[i]);
        }
        qdcT += round(QDCSums[i]);
      }
      qdcP = qdcS/qdcF * 32768;
      
      Measurement qdcFast("TraceQDCFast", qdcF);
      retval.push_back(qdcFast);

      Measurement qdcSlow("TraceQDCSlow", qdcS);
      retval.push_back(qdcSlow);
      
      Measurement qdcPID("TraceQDCPID", qdcP  );
      retval.push_back(qdcPID);

      Measurement qdcTotal("TraceQDCTotal", qdcT);
      retval.push_back(qdcTotal);

      if (qdcT > enLo && qdcT < enHi && qdcP > pidLo && qdcP < pidHi) { good_trace = true; }
      else{good_trace=false;}      

      return retval;
    }
  
    int TrapezoidQDC::dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append, std::string traceName){
      //Open files

#if ROOT_COMPILE == 1
      TFile *outFile;
      if (append==1){
        outFile = new TFile(fileName.c_str(), "UPDATE");
      }
      else{
        outFile = new TFile(fileName.c_str(), "RECREATE");
      }

      double max = 1;
      TH1D *HTrace=new TH1D((traceName+std::to_string(n_traces)).c_str(),(traceName+std::to_string(n_traces)).c_str(),traceLen,0,traceLen);
      for(int i=0;i<traceLen;i++){
        HTrace->AddBinContent(i,trace[i]-trace[0]);
        if (trace[i]-trace[0]>max){
          max = trace[i]-trace[0];
        }
      }
      HTrace->Write();      

      //Draw QDC windows
      TMultiGraph *mg = new TMultiGraph("QDCS","QDCS");
      float nextQDC=0;
      for(int i=0;i<8;i++){
        TGraph *gr = new TGraph();
        gr->SetLineColor(kRed);
        gr->SetLineWidth(2);
        nextQDC +=QDCWindows[i];
        gr->SetPoint(0,nextQDC,0);
        gr->SetPoint(1,nextQDC,0.1*max);
        mg ->Add(gr);
      }
      mg->Write();

      //Close files
      outFile->Purge();
      outFile->Close();
#endif 
     
      return 1;
    }

    //PeakTail
    void PeakTail::Load(const char *filename, int index) {
      FILE *fpr = fopen(filename, "r");
      if (fpr!=NULL){
        char line[200];
        int ind=-1;
        while (fgets(line, 200, fpr)!=0) {
          switch(line[0]){
          case ' ':
          case '\t':
          case '\n':
          case '#':
            break;
          default:
            sscanf(line,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                   &ind,&bLow,&bHigh,&pLow,&pHigh,&tLow,&tHigh,&eLow,
                   &eHigh,&pbLow,&pbHigh,&fL,&fG,&ffThr);
          }
          if (ind == index) { break; }
        }
        fclose(fpr);
        is_loaded = true;
      }
      else {
        printf("%sUnable to open file %s, traces will not be processed%s\n",ANSI_COLOR_RED, filename, ANSI_COLOR_RESET);
      }
    }

    std::vector<Measurement> PeakTail::Prototype() {
      std::vector<Measurement> retval;
      Measurement energy("energy", 0);
      Measurement peak("peak", 0);
      Measurement tail("tail", 0);

      retval.push_back(energy);
      retval.push_back(peak);
      retval.push_back(tail);

      return retval;
    }

    std::vector<Measurement> PeakTail::Process(uint16_t *trace, int length) {
      good_trace = true;
      std::vector<Measurement> retval;
      //actual trace processing
      //float BL[length];//={0};
      //float fD[length];//={0};
      //float fTrap[length];//={0};

      float background = 0.0;
      for (int i=bLow; i<=bHigh; ++i) {
        background += trace[i];
      }
      background /= (bHigh - bLow + 1);

      float energy = 0.0;
      for (int i=eLow; i<=eHigh; ++i) {
        energy += trace[i] - background;
      }

      float peak = 0.0;
      for (int i=pLow; i<=pHigh; ++i) {
        peak += trace[i] - background;
      }

      float tail = 0.0;
      for (int i=tLow; i<=tHigh; ++i) {
        tail += trace[i] - background;
      }

      float post_background = 0.0;
      for (int i=pbLow; i<=pbHigh; ++i) {
        post_background += trace[i];
      }
      post_background /= (pbHigh - pbLow + 1);

      /*
        int ffTrig = 0;
        for (int k=0;k<length;k++) {
        BL[k]=trace[k]-background;

        fD[k]=BL[k] - (k-fL>=0)*BL[k-fL] - ((k-(fG+fL))>=0)*BL[k-(fG+fL)] + ((k-(fG + 2*fL))>=0)*BL[k-(fG + 2*fL)];
        fTrap[k] = (k-1>=0)*fTrap[k-1] + fD[k];

        //Find fast trigger and CFD values                                    
        if (fTrap[k-1]/(float)fL<=ffThr && fTrap[k]/(float)fL>ffThr) {
        ffTrig+=1;
        }
        }
      */
      retval.emplace_back("energy", (int)energy);
      retval.emplace_back("peak", (int)peak);
      retval.emplace_back("tail", (int)tail);
      retval.emplace_back("background", (int)background);
      retval.emplace_back("post_background", (int)post_background);
      //retval.emplace_back("nTrigs", ffTrig);

      return retval;
    }

    int PeakTail::dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName){
#if ROOT_COMPILE == 1
      TFile *outFile;
      if (append==1){
        outFile = new TFile(fileName.c_str(), "UPDATE");
      }
      else{
        outFile = new TFile(fileName.c_str(), "RECREATE");
      }
      
      //write trace + anything else to file
      TH1D *HTrace=new TH1D((traceName+std::to_string(n_traces)).c_str(),(traceName+std::to_string(n_traces)).c_str(),traceLen,0,traceLen);
      for(int i=0;i<traceLen;i++){
        HTrace->AddBinContent(i,trace[i]-trace[0]);
      }
      HTrace->Write();
      
      //Close File
      outFile->Purge();
      outFile->Close();
#endif
      return 0;
    }

    //QDCMaxVal
    void QDCMaxVal::Load(const char *filename, int index) {
      FILE *fpr = fopen(filename, "r");
      if (fpr!=NULL){
        char line[200];
        int ind=-1;
        while (fgets(line, 200, fpr)!=0) {
          switch(line[0]){
          case ' ':
          case '\t':
          case '\n':
          case '#':
            break;
          default:
            sscanf(line,"%d %d %d %d %d",
                   &ind,&bLow,&bHigh,&pLow,&pHigh);
          }
          if (ind == index) { break; }
        }
        fclose(fpr);
        is_loaded = true;
      }
      else {
        printf("%sUnable to open file %s, traces will not be processed%s\n",ANSI_COLOR_RED, filename, ANSI_COLOR_RESET);
      }
    }

    std::vector<Measurement> QDCMaxVal::Prototype() {
      std::vector<Measurement> retval;
      Measurement peak("peak", 0);
      Measurement background("background", 0);
      Measurement maxval("maxval", 0);

      retval.push_back(peak);
      retval.push_back(background);
      retval.push_back(maxval);

      return retval;
    }

    std::vector<Measurement> QDCMaxVal::Process(uint16_t *trace, int length) {
      good_trace = true;
      std::vector<Measurement> retval;

      float background = 0.0;
      for (int i=bLow; i<=bHigh; ++i) {
        background += trace[i];
      }
      background /= (float)(bHigh - bLow + 1.);

      float maxVal = 0.0;
      float peak = 0.0;
      for (int i=pLow; i<=pHigh; ++i) {
        peak += (float)trace[i] - background;
        if (trace[i] > maxVal) { maxVal = trace[i]; }
      }

      maxVal -= background;

      retval.emplace_back("peak", (int)peak);
      retval.emplace_back("background", (int)background);
      retval.emplace_back("maxval", (int)maxVal);

      return retval;
    }

    int QDCMaxVal::dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName){
#if ROOT_COMPILE == 1
      TFile *outFile;
      if (append==1){
        outFile = new TFile(fileName.c_str(), "UPDATE");
      }
      else{
        outFile = new TFile(fileName.c_str(), "RECREATE");
      }
      
      //write trace + anything else to file
      TH1D *HTrace=new TH1D((traceName+std::to_string(n_traces)).c_str(),(traceName+std::to_string(n_traces)).c_str(),traceLen,0,traceLen);
      for(int i=0;i<traceLen;i++){
        HTrace->AddBinContent(i,trace[i]-trace[0]);
      }
      HTrace->Write();
      
      //Close File
      outFile->Purge();
      outFile->Close();
#endif
      return 0;
    }
    
    
  }
}
