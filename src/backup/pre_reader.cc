#include "event.hh"

#include "pre_reader.hh"

#include <iostream>

namespace PIXIE {
  
  off_t PreReader::offset() const
  {
    return (ftello(this->file));
  }

  int PreReader::open(const std::string &path)
  {
    if (this->file)
      {
        return (-1); //file has already been opened
      }

    if (!(this->file = fopen(path.c_str(), "rb")))
      return (-2);

    return (0);
  }

  int PreReader::read(size_t breakatevent) {
    struct stat stat;
    if (fstat(fileno(this->file), &stat) != 0)
      std::cout << "Couldn't stat" << std::endl;

    this->fileLength = stat.st_size;
    
    offsets.clear();

    int thread = 0;
    int events=0;
    while (true) {
      //if (breakatevent==0){
      if (thread==this->nThreads) { break; }
      if (offset()*this->nThreads >= thread*this->fileLength) {
	offsets.push_back(offset());
	++thread;
      }
      //}
    //else{
    //	printf("here %d %d\n",thread,nThreads);
    //	if (thread>this->nThreads) { break; }
    //	if (events*this->nThreads >= thread*breakatevent) {
    //	  offsets.push_back(offset());
    //	  ++thread;
    //	}
    //}
      uint32_t firstWord;
      if (fread(&firstWord, (size_t) 4, (size_t) 1, this->file) != 1) {
        return -1;    
      }
      events+=1;
      Mask mEventLength = Mask(0x7FFE0000, 17);
      
      Mask mChannelNumber = Mask(0xF, 0);
      Mask mSlotID = Mask(0xF0, 4);
      Mask mCrateID = Mask(0xF00, 8);
      Mask mHeaderLength = Mask(0x1F000, 12);
      
      uint32_t eventLength = mEventLength(firstWord);

      /*
      uint32_t channelNumber=mChannelNumber(firstWord);
      uint32_t slotID=mSlotID(firstWord);
      uint32_t crateID=mCrateID(firstWord);
      uint32_t headerLength=mHeaderLength(firstWord);

      bool qdcs = false;
      bool eraw = false;
      bool traces = false;
      if (headerLength == 8) { eraw = true; }
      else if (headerLength == 12) { qdcs = true; }
      else if (headerLength == 16) { qdcs = true; eraw = true; }
      if (headerLength != eventLength) { traces = true; }

      definition.AddChannel(crateID, slotID, channelNumber, eraw, qdcs,traces);
      */
      
      if (fseek(this->file, (eventLength-1)*4, SEEK_CUR)) {
        return -1;
      }      
    }
    return 0;
  }

  void PreReader::print() const {
    std::cout << "File size " << this->fileLength << std::endl;
    for (int i=0; i<offsets.size(); ++i) {
      std::cout << "Thread " << i << "    " << offsets[i] << std::endl;
    }
  }
}
