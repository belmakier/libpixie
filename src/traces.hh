#ifndef PIXIE_TRACES_HH
#define PIXIE_TRACES_HH

namespace PIXIE {
  namespace Trace {
    class Measurement {
    public:
      std::string name; //for a branch name
      int32_t datum;  //arbitrarily 32-bit int, should be general enough
      
      Measurement() : datum(0) {};
      Measurement(std::string n, int32_t d) : name(n), datum(d) {}
    };

    class Algorithm {
    public:
      bool good_trace;
      int loaded=false;
      
      virtual void Load(const char *filename, int index) = 0; //for loading parameters
      virtual std::vector<Measurement> Process(uint16_t *trace, int length) = 0; //pure virtual
      virtual std::vector<Measurement> Prototype() = 0; //returns prototype - same size + names as Process() but with all datums = 0, used to initialise the tree
      virtual int dumpTrace(uint16_t *trace, int traceLen, int n_traces, std::string fileName, int append,std::string traceName) = 0 ;
      
    };
  }
}

#endif
