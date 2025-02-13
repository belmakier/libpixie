#include "nsclreader.hh"
#include "reader.hh"
#include <string.h>

namespace PIXIE {
  int NSCLReader::readRingItemHeader(Reader *reader) {
#if PIXIE_READTYPE==0
      if (fread(&ri_size, (size_t)sizeof(int), 1, reader->file) != 1) {
        return -1;
      }
      if (fread(&ri_type, (size_t)sizeof(int), 1, reader->file) != 1) {
        return -1;
      }
#elif PIXIE_READTYPE==1||PIXIE_READTYPE==2
      memcpy(&ri_size, &reader->buffer[reader->off], sizeof(char)*4);
      reader->off += 4;
      memcpy(&ri_type, &reader->buffer[reader->off], sizeof(char)*4);
      reader->off += 4;
      if (reader->off > reader->fileSize) { return -1; }
#endif
    
    if (ri_type != 30 ) {
      //non-physics item
      if (ri_type == 1) {
        std::cout << std::endl;
        std::cout << "============== BEGIN RUN ===============" << std::endl;
      }
      else if (ri_type == 2) {
        std::cout << std::endl;
        std::cout << "=============== END RUN ================" << std::endl;
      }
      else if (ri_type == 3) {
        std::cout << std::endl;
        std::cout << "============== PAUSE RUN ===============" << std::endl;
      }
      else if (ri_type == 4) {
        std::cout << std::endl;
        std::cout << "============= RESUME RUN ===============" << std::endl;
      }
      else if (ri_type == 20) {
        
        //std::cout << std::endl;
        //std::cout << "SCALERS FOUND..." << std::endl;
       
      }
      else {
#ifdef DEBUG         
        std::cout << std::endl;
        std::cout << "Untreated Ring item: " << std::endl;
        std::cout << ri_size << "   " << ri_type << std::endl;
#endif        
      }
    }
#ifdef DEBUG
    std::cout << "rih size = " << ri_size << std::endl;
#endif
    return ri_type;
  }

  int NSCLReader::readRingItemBodyHeader(Reader *reader) {
    uint32_t ribh_size;
    uint64_t ribh_ts;
    uint32_t ribh_sid;
    uint32_t ribh_bt;
#if PIXIE_READTYPE==0
      if (fread(&ribh_size, (size_t)sizeof(uint32_t), 1, reader->file) != 1) {
        return -1;
      }      
#elif PIXIE_READTYPE==1||PIXIE_READTYPE==2
      memcpy(&ribh_size, &reader->buffer[reader->off], sizeof(uint32_t));
      reader->off+=sizeof(uint32_t);
      if (reader->off > reader->fileSize) { return -1; }
#endif
      
#ifdef DEBUG
    std::cout << "Ring Item Body Header: " << std::endl;
    std::cout << "           size : " << ribh_size << std::endl;
#endif

    if (ribh_size == 0) { presort = true; return 0; }

#if PIXIE_READTYPE==0
      if (fread(&ribh_ts, (size_t)sizeof(uint64_t), 1, reader->file) != 1) {
        return -1;
      }
      if (fread(&ribh_sid, (size_t)sizeof(int), 1, reader->file) != 1) {
        return -1;
      }
      if (fread(&ribh_bt, (size_t)sizeof(int), 1, reader->file) != 1) {
        return -1;
      }
#else      
      reader->off += sizeof(uint64_t) + sizeof(int) + sizeof(int);
#endif

    //std::cout << "      timestamp : " << ribh_ts << std::endl;
    //std::cout << "      source ID : " << ribh_sid << std::endl;
    //std::cout << "   barrier type : " << ribh_bt << std::endl;
    
    
    return ribh_size;
  }
  
  int NSCLReader::readRingItemBody(Reader *reader) {
#if PIXIE_READTYPE==0
    if (fread(&rib_size, (size_t)sizeof(int), 1, reader->file) != 1) {
      return -1;
    }
#elif PIXIE_READTYPE==1||PIXIE_READTYPE==2
    memcpy(&rib_size, &reader->buffer[reader->off], sizeof(char)*4);
    reader->off+=4;
    if (reader->off > reader->fileSize) { return -1; }
#endif

#ifdef DEBUG
    std::cout << "Reading ring item body size = " << rib_size << std::endl;
#endif
    
    rib_size -= 4;
    return rib_size;
  }

  int NSCLReader::readNextFragment(Reader *reader) {
    //this skips over the fragment header, the ring item header, and the ring item body header associated with the fragment
    //reads the first 2 words of the ring item body (body size + device info)
    //leaves the pointer right at the beginning of the actual PIXIE fragment
    if (!presort) {
      if (rib_size > 48) {
#ifdef DEBUG
        std::cout << "rib_size = " << rib_size << std::endl;
#endif 
        /*
        uint64_t frag_ts;
        uint32_t frag_sid;
        uint32_t frag_paysize;
        uint32_t frag_bt;

        uint32_t frag_rih_size;
        uint32_t frag_rih_type;
      
        uint32_t frag_ribh_size;
        uint64_t frag_ribh_ts;
        uint32_t frag_ribh_sid;
        uint32_t frag_ribh_bt;

        if (fread(&frag_ts, (size_t)sizeof(uint64_t), 1, file) != 1) { return -1; }
        if (fread(&frag_sid, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        if (fread(&frag_paysize, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        if (fread(&frag_bt, (size_t)sizeof(int), 1, file) != 1) { return -1; }

        if (fread(&frag_rih_size, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        if (fread(&frag_rih_type, (size_t)sizeof(int), 1, file) != 1) { return -1; }

        if (fread(&frag_ribh_size, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        if (fread(&frag_ribh_ts, (size_t)sizeof(uint64_t), 1, file) != 1) { return -1; }
        if (fread(&frag_ribh_sid, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        if (fread(&frag_ribh_bt, (size_t)sizeof(int), 1, file) != 1) { return -1; }
        */
#if PIXIE_READTYPE==0
        fseek(reader->file, 48, SEEK_CUR);  //skip the rest of the non-physics event
#elif PIXIE_READTYPE==1 || PIXIE_READTYPE==2
        reader->off += 48;
#endif
        /*
        std::cout << "Fragment Header: " << std::endl;
        std::cout << "       Timestamp: " << frag_ts << std::endl;
        std::cout << "       Source ID: " << frag_sid << std::endl;
        std::cout << "    Payload Size: " << frag_paysize << std::endl;
        std::cout << "    Barrier Type: " << frag_bt << std::endl;

        std::cout << "Fragment RIH: " << std::endl;
        std::cout << "            Size: " << frag_rih_size << std::endl;
        std::cout << "            Type: " << frag_rih_type << std::endl;

      
        std::cout << "Fragment RIBH: " << std::endl;
        std::cout << "            Size: " << frag_ribh_size << std::endl;
        std::cout << "       Timestamp: " << frag_ribh_ts << std::endl;
        std::cout << "       Source ID: " << frag_ribh_sid << std::endl;
        std::cout << "    Barrier Type: " << frag_ribh_bt << std::endl;
        */
      
        uint32_t frag_size;  //in 16 bit words not 32 bit words
        uint32_t dev_info;
#if PIXIE_READTYPE==0
          if (fread(&frag_size, (size_t)sizeof(int), 1, reader->file) != 1) {
            return -1;
          }
          if (fread(&dev_info, (size_t)sizeof(int), 1, reader->file) != 1) {
            return -1;
          }
#elif PIXIE_READTYPE==1 || PIXIE_READTYPE==2
          memcpy(&frag_size, &reader->buffer[reader->off], sizeof(char)*4);
          reader->off+=4;
          memcpy(&dev_info, &reader->buffer[reader->off], sizeof(char)*4);
          reader->off+=4;
          if (reader->off > reader->fileSize) { return -1; }
#endif
      
        uint32_t ADC_freq = (dev_info & 0x0000ffff);
        uint32_t ADC_res = (dev_info & 0xff0000) >> 16;
        uint32_t mod_rev = (dev_info & 0xff000000) >> 24;

#ifdef DEBUG
        std::cout << "dev_info = " << ADC_freq << "   " << ADC_res << "  " << mod_rev << std::endl;
           
        std::cout << "frag_size = " << frag_size << std::endl;
#endif 
      
        rib_size -= (48 + frag_size*2);
        if (rib_size < 0 ) {
          std::cout << "SEVERE ERROR, NAVIGATION THROUGH RING BUFFER BROKEN" << std::endl;
          return -1;
        }
      }
      else if (rib_size < 48) {
        std::cout << "REMAINING DATA: " << std::endl;
        int rib_tmp = rib_size;
        int data;
        while (rib_tmp > 0) {
#if PIXIE_READTYPE==0
          fread(&data, (size_t)sizeof(int), 1, reader->file);
#elif PIXIE_READTYPE==1 || PIXIE_READTYPE==2
          memcpy(&data, &reader->buffer[reader->off], sizeof(char)*4);
          reader->off+=4;
          if (reader->off > reader->fileSize) { return -1; }
#endif
          rib_tmp -= 4;
        }
      }
      else {
        std::cout << "Warning! readNextFragment called when rib_size == 0" << std::endl;
      }
    }
#ifdef DEBUG
    std::cout << "readNextFragment returning " << rib_size << std::endl;
#endif
    return rib_size;      
  }

  int NSCLReader::findNextFragment(Reader *reader) {
    if (rib_size > 0) {
      return readNextFragment(reader);
    }

    while (true) {
      int type = readRingItemHeader(reader);
      if (type == -1) { return -1; }

#ifdef DEBUG
      std::cout << "type = " << type << std::endl;
      std::cout << "size = " << ri_size << std::endl;
#endif 
      /*
      if (type >= 1 && type <= 4) {
        uint32_t nRun;
        uint32_t toff;
        uint32_t wall_time;
        uint32_t off_div;
        char title[80];
        fread(&nRun, (size_t)sizeof(int), 1, file);
        fread(&toff, (size_t)sizeof(int), 1, file);
        fread(&wall_time, (size_t)sizeof(int), 1, file);
        fread(&off_div, (size_t)sizeof(int), 1, file);
        fread(&title[0], (size_t)sizeof(char), 80, file);

        time_t wall_timet = (time_t)wall_time;
        struct tm* timeinfo;
        timeinfo = localtime(&wall_timet);
        
        
        std::cout << "                 Run: " << nRun << std::endl;
        std::cout << "Time since run start: " << toff << "s" << std::endl;
        printf(      "                Time: %s\n", asctime(timeinfo));
        std::cout << "               Title: " << title << std::endl;        
      } 
      */       
      
      if (type != 30) {
#if PIXIE_READTYPE==0
        fseek(reader->file, ri_size-8, SEEK_CUR);  //skip the rest of the non-physics event
#elif PIXIE_READTYPE==1||PIXIE_READTYPE==2
        reader->off += (ri_size-8);
#endif
        continue;
      }
      int ribh_size = readRingItemBodyHeader(reader);
      if (ribh_size < 0) { return ribh_size; }

      if (!presort) {
        readRingItemBody(reader);
      }
      else {
        uint32_t tmp[2];
#if PIXIE_READTYPE==0
        std::fread(&tmp[0], 4, 2, reader->file);
#elif PIXIE_READTYPE==1||PIXIE_READTYPE==2
        reader->off += 8;
#endif
        rib_size = ri_size-20;
      }
      if (rib_size < 0) { return rib_size; }

      return readNextFragment(reader);      
    }
  }
}    
