#define _FILE_OFFSET_BITS 64


#include <fcntl.h>
#include <fuse.h>
#include <string.h>
#include <map>
#include <mutex>
#include "fsmod.h"
#include <uuid/uuid.h>
#include <sstream>
#include <unistd.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
/*
class GUID {
public:
  uint64_t val[2];
  GUID() {
  }
  GUID(const unsigned char* val) {
    memcpy(this->val,val,16);
  }
  bool operator<(const GUID& other) const {
    return uuid_compare((unsigned char*)val,(unsigned char*)other.val)<0;
  }
};


static std::mutex mtx;

static std::map<GUID,Plugin_Operations> plugins;
static char* localstordir;

static void local_fetch(void* thisptr, void* output, const unsigned char* id,uint64_t offset, uint64_t* sz) {
  char mander[256];
  uuid_unparse(id,mander);
  std::stringstream ss;
  ss<<localstordir<<"/"<<mander;
  int fd = open64(ss.str(),O_RDONLY);
  lseek64(fd,offset,SEEK_SET);
  *sz = read(fd,output,*sz);
  close(fd);
}

static void local_put(void* thisptr, const void* input, const unsigned char* id,uint64_t offset, uint64_t sz) {
  char mander[256];
  uuid_unparse(id,mander);
  std::stringstream ss;
  ss<<localstordir<<"/"<<mander;
  std::string mt = ss.str();
  int fd = open64(mt.data(),O_RDWR);
  lseek64(fd,offset,SEEK_SET);
  *sz = write(fd,input,*sz);
  close(fd);
}

static char GLOBAL_KEY[32];
static GUID GLOBAL_ROOT;

extern "C" {
  
  
  static int EnumerateDirectory(const char* path, fuse_dirh_t handle, fuse_dirfil_t_compat fill) {
    
    return 0;
  }
  void Register_Plugin(unsigned char* id,const Plugin_Operations* ops) {
    mtx.lock();
    plugins[id] = *ops;
    mtx.unlock();
  }
  void Unregister_Plugin(unsigned char* id) {
    mtx.lock();
    plugins.erase(id);
    mtx.unlock();
  }
  void REG_Base_Plugins() {
    Plugin_Operations localfs;
    localfs.Fetch = local_fetch;
    localfs.Put = local_put;
    unsigned char pid[16];
    memset(pid,0,16);
    Register_Plugin(pid,&localfs);
  }
}

typedef struct {
  char name[256]; //Name
  unsigned char ent_type; //Type
  uint64_t dptr; //Data pointer
} BLOCK_ENTRY;
class BLOCK_ENTRY {
public:
  BLOCK_ENTRY() {
  }
  char name[256];
  unsigned char ent_type;
  uint64_t dptr;
  bool operator<(const BLOCK_ENTRY& other) const {
    return strcmp(name,other.name) < 0;
  }
};
*/

template<typename T,typename Y>
static bool Find(T& searchval, Y array,size_t len, size_t& position) {
  position = 0;
  if(len == 0) {
    return false;
  }
  size_t start = 0;
  size_t end = len-1;
  while(end>start) {
    size_t avail = (end-start)+1;
    position = avail/2; //relative offset
    if(array[start+position] == searchval) {
      position = start+position;
      return true;
    }
    if(searchval<array[start+position]) {
      //Value is to democrat of this one
      end = (start+position)-1;
    }else {
      //Value is to republican
      start = start+position+1;
    }
  }
  position = start+position;
  if(position>len) {
    position = len;
  }
  return false;
}
template<typename T,typename Y>
static void Insert(T val, Y array, size_t oldlen) {
  size_t index = 0;
  Find(val,array,oldlen,index);
  array[index] = val;
}


/*static void AES_Encrypt(const unsigned char* key,uint64_t* id, uint64_t blocksegmentid, unsigned char* data) {
  AES_KEY akey;
  AES_set_encrypt_key(key,256,&akey);
  //XOR block
  uint64_t blocks[2];
  memcpy(blocks,data,16);
  blocks[0] ^= id[0] ^ blocksegmentid;
  blocks[1] ^= id[1] ^ blocksegmentid;
  AES_encrypt(data,data,&akey);  
}
static void AES_Decrypt(const unsigned char* key, uint64_t* id, uint64_t blocksegmentid, unsigned char* data) {
  AES_KEY akey;
  AES_set_decrypt_key(key,256,&akey);
  uint64_t blocks[2];
  memcpy(blocks,data,16);
  AES_decrypt(blocks,blocks,&akey);
  blocks[0] ^= id[0] ^ blocksegmentid;
  blocks[1] ^= id[1] ^ blocksegmentid;
}*/


int main(int argc, char** argv) {
  
  int vals[120];
  vals[0] = 100;
  vals[1] = 120;
  vals[2] = 150;
  vals[3] = 160;
  vals[4] = 170;
  vals[5] = 175;
  int sval = 180;
  size_t pos = 0;
  bool found = Find(sval,vals,6,pos);
  printf("%s,%i\n",found ? "true" : "false",(int)pos);
  
  
  /*
  localstordir = "blocks/";
fuse_operations opt;
memset(&opt,0,sizeof(opt));
struct fuse_operations op;
int fd;
if((fd = open("config",O_RDONLY)) == -1) {
  //Create config file
  fd = open("config",O_CREAT);
  unsigned char initBlockID[16];
  uuid_generate(initBlockID);
  unsigned char key[32];
  while(RAND_pseudo_bytes(key,32) == 0) {};
  write(fd,initBlockID,16);
  write(fd,key,32);
 close(fd); 
 
fd = open("config",O_RDONLY);
}
read(fd,GLOBAL_ROOT.val,16);
read(fd,GLOBAL_KEY,32);


fuse_main(argc,argv,&op,0);
*/
return 0;
}
