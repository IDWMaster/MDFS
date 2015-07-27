#include <stddef.h>
#include <stdint.h>
typedef struct {
  uint64_t block[2];
  uint64_t offset;
  uint64_t count;
} WORK_ORDER;

extern "C" {
  //Requests a block work request from the filesystem driver
  void GetWork(WORK_ORDER* output);
  //Marks a work request as completed and verified. Each plugin is responsible for verifying the integrity of each completed work item.
  //Plugins that work over remote networks should take special care to cryptographically protect data; using mechanisms such as SHA
  //hashing and cryptographic signatures, if applicable. Each agent should ensure that all blocks are also the most recent version.
  void CompleteWork(WORK_ORDER* order, const void* data, uint64_t len);
}