/* This file contains the details of every algorithm that could be used to process the traces.  

   You should edit/add to this file, the corresponding .cc file if you want to add your own algorithm.

   Create a class that inherits from the PIXIE::Trace::Algorithm base class, and make sure you overload the Load, Process, and Prototype methods.  Load should take a settings file and load appropriate settings into your object.  Process is a method that actually processes the traces, outputting a vector PIXIE::Trace::Measurement objects - these must always be in the same order, and the method should always return the same number of them.  Prototype() returns an equivalent vector, but with no trace needed.  It is used only to determine how many measurements the particular algorithm returns, and get the names of the measurements for the Tree branch names.

   If you're confused or don't understand, send me an email at <timothy.gray@anu.edu.au>
*/

#ifndef PIXIE_TRACE_ALGS_HH
#define PIXIE_TRACE_ALGS_HH

#include "traces.hh"

namespace PIXIE {
  int setTraceAlg(PIXIE::Trace::Algorithm *&tracealg, std::string algName, std::string algFile, int algIndex);
  
  //////////////////////////
  // EACH ALGORITHM CLASS //
  //////////////////////////
  namespace Trace {
    class Trapezoid : public Algorithm {
    public:
      //these paramters are set by load
      int fL;
      int fG;
      int sL;
      int sG;
      float tau;
      int D;
      int S;
      float ffThr;
      float cfdThr;      

      void Load(const char *file, int index); //for loading parameters
      std::vector<Measurement> Process(uint16_t *trace, int length); //pure virtual      
      std::vector<Measurement> Prototype();
      int dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName);

      std::vector<Measurement> TrapFilter(uint16_t *trace, int length, int write = 0);
      double  GetBaseline(uint16_t *trace, int length);
    };

    class TrapezoidQDC : public Trapezoid {  //trapezoid with QDC windows as well
    public:
      float QDCWindows[8];
      char stringSlow[9],stringFast[9];
      float enLo=0,enHi=999999999;
      float pidLo=0,pidHi=999999999;

      void Load(const char *file, int index); //for loading parameters
      std::vector<Measurement> Process(uint16_t *trace, int length); //pure virtual
      std::vector<Measurement> Prototype();
      int dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName);
    };

    class PeakTail : public Algorithm {
    public:
      //these paramters are set by load
      int bLow;
      int bHigh;
      int pLow;
      int pHigh;
      int tLow;
      int tHigh;
      int eLow;
      int eHigh;
      int pbLow;
      int pbHigh;
      int fL;
      int fG;
      int ffThr;

      void Load(const char *file, int index); //for loading parameters
      std::vector<Measurement> Process(uint16_t *trace, int length); //pure virtual      
      std::vector<Measurement> Prototype();
      int dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName);
    };


  }
  

}

#endif
