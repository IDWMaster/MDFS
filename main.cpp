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
#include <GGRouter.h>


namespace MDFS {
class GUID {
public:
  uint64_t val[2];
  GUID() {
  }
  GUID(const unsigned char* val) {
    memcpy(this->val,val,16);
  }
  GUID(const uint64_t* val) {
    this->val[0] = val[0];
    this->val[1] = val[1];
  }
  bool operator<(const GUID& other) const {
    return uuid_compare((unsigned char*)val,(unsigned char*)other.val)<0;
  }
};


static std::mutex mtx;

static char* localstordir;

static bool local_fetch(void* thisptr, void* output, const unsigned char* id,uint64_t offset, uint64_t* sz) {
  char mander[256];
  uuid_unparse(id,mander);
  std::stringstream ss;
  ss<<localstordir<<"/"<<mander;
  std::string mm = ss.str();
  int fd = open64(mm.data(),O_RDONLY);
  lseek64(fd,offset,SEEK_SET);
  *sz = read(fd,output,*sz);
  close(fd);
}

static bool local_put(void* thisptr, const void* input, const unsigned char* id,uint64_t offset, uint64_t sz) {
  char mander[256];
  uuid_unparse(id,mander);
  std::stringstream ss;
  ss<<localstordir<<"/"<<mander;
  std::string mt = ss.str();
  int fd = open64(mt.data(),O_RDWR);
  lseek64(fd,offset,SEEK_SET);
  write(fd,input,sz);
  close(fd);
  return true;
}

static char GLOBAL_KEY[32];
static GUID GLOBAL_ROOT;



class BlockRequest {
public:
  GUID block;
  uint64_t offset;
  uint64_t len;
  BlockRequest() {
  }
  bool operator<(const BlockRequest& other) const {
    return memcmp(&other,this,sizeof(this)) < 0;
  }
  bool operator==(const BlockRequest& other) const {
    return memcmp(&other,this,sizeof(this)) == 0;
  }
};

//Pending request list

static std::map<BlockRequest,GGClient::WaitHandle*> pendingRequests;
static GGClient::WaitHandle workEvent;


static GGClient::WaitHandle* RequestBlockSegment(const BlockRequest& req) {
  mtx.lock();
  auto bot = new GGClient::WaitHandle();;
  pendingRequests[req] = bot;
  workEvent.Signal();
  mtx.unlock();
  return bot;
}



static bool RetrieveBlock(const GUID& block, uint64_t position, uint64_t len, void* output) {
  BlockRequest req;
  req.block = block;
  req.offset = position;
  req.len = len;
  GGClient::WaitHandle* wh = RequestBlockSegment(req);
  wh->Fetch();
  if(wh->data) {
    memcpy(output,wh->data,len);
  }
  wh->Unfetch();
  return wh->data;
  
}







extern "C" {
  
  
  void GetWork(WORK_ORDER* output) {
    bool found = false;
    BlockRequest request;
    mtx.lock();
    if(!pendingRequests.empty()) {
      found = true;
      request = pendingRequests.begin()->first;
    }
    mtx.unlock();
    if(found) {
      memcpy(output->block,request.block.val,16);
      output->offset = request.offset;
      output->count = request.len;
    }
  }
  void CompleteWork(WORK_ORDER* order, const void* data, uint64_t len) {
    BlockRequest request;
    memcpy(request.block.val,data,16);
    request.offset = order->offset;
    request.len = order->count;
    mtx.lock();
    auto bot = pendingRequests.find(request);
    if(bot != pendingRequests.end()) {
      //Signal WORK_ORDER
      bot->second->Put((unsigned char*)data,len);
    }
    mtx.unlock();
  }
  
  
  
}

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

template<typename T>
static bool Find(T searchval, T* array,size_t len, size_t& position) {
  T* origpos = array;
  position = 0;
  if(len == 0) {
    return false;
  }
  while(len>0) {
    position = len/2;
    if(array[position] == searchval) {
      return true;
    }
    if(searchval<array[position]) {
      len=position;
    }else {
      array+=position+1;
      len-=position+1;
    }
  }
  position = array-origpos;
}
template<typename T>
static void Insert(T val, T* array, size_t oldlen) {
  
  
  
  size_t marker = 0;
  Find(val,array,oldlen,marker);
  //Insert at insertion marker, move everything to the right
  memmove(array+marker+1,array+marker,(oldlen-marker)*sizeof(val));
  array[marker] = val;
}


static void AES_Encrypt(const unsigned char* key,uint64_t* id, uint64_t blocksegmentid, unsigned char* data) {
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
  AES_decrypt((unsigned char*)blocks,(unsigned char*)blocks,&akey);
  blocks[0] ^= id[0] ^ blocksegmentid;
  blocks[1] ^= id[1] ^ blocksegmentid;
}





//TODO: Hash data into blocks of 4KB using AES-CBC as hashing algorithm to prevent manipulation of data
//Last 4KB of data in each chunk will be all zeroes. If the data is NOT all zeroes; it indicates the block is corrupt.


class EncryptedStream {
public:
  //Base GUID
  GUID baseIndex;
  unsigned char key[32];
  EncryptedStream(const GUID& baseGuid, const unsigned char* key) {
    baseIndex = baseGuid;
    memcpy(this->key,key,32);
    
  }
  
  
  
};



static std::string GetLocalPath(const GUID& guid) {
  
    std::stringstream ss;
    char mander[256];
    uuid_unparse((unsigned char*)guid.val,mander);
    ss<<localstordir<<"/"<<mander;
    return ss.str();
}
  static void GetFSPointer(const char* path, GUID& output) {
    std::string local_root = GetLocalPath(GLOBAL_ROOT);
    char* path_temp = new char[strlen(path)+1];
    memcpy(path_temp,path,strlen(path)+1);
    
    delete[] path_temp;
  }
  
  static int EnumerateDirectory(const char* _path, fuse_dirh_t handle, fuse_dirfil_t_compat fill) {
    //Get GLOBAL_ROOT
    //localstordir
    std::string path = GetLocalPath(GLOBAL_ROOT);    
    return 0;
  }

static int Main(int argc, char** argv) {
  localstordir = "blocks/";
fuse_operations opt;
memset(&opt,0,sizeof(opt));
opt.getdir = EnumerateDirectory;
int fd;
if((fd = open("config",O_RDONLY)) == -1) {
  //Create config file
  fd = open("config",O_CREAT | O_RDWR, S_IRUSR | S_IRGRP);
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
fuse_main(argc,argv,&opt);

return 0;

}

}




int main(int argc, char** argv) {
  
  return MDFS::Main(argc,argv);
  
  }