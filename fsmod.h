#include <stddef.h>
typedef struct Plugin_Operations {
  void* thisptr;
  bool(*Fetch)(void*,void*,const unsigned char*,uint64_t,uint64_t*); //thisptr, output,block ID,offset, amount of data to read (also output)
  bool(*Put)(void*,const void*,const unsigned char*,uint64_t,uint64_t); //thisptr,input,block ID,offset,amount of data to write
  
};

extern "C" {
  void Register_Plugin(unsigned char* id,const Plugin_Operations* ops);
  void Unregister_Plugin(unsigned char* id);
}