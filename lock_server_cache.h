#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include <queue>


class lock_server_cache {
 private:
pthread_mutex_t m;
  int nacquire;
struct lock{
  std::string id;
  bool stat;
  std::queue<std::string> q;
};
  std::map<lock_protocol::lockid_t, lock> locks; 
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
