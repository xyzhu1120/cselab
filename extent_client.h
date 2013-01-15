// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <map>
#include <pthread.h>
#include "extent_protocol.h"
#include "rpc.h"

class extent_client {
 private:
  pthread_mutex_t m;
  rpcc *cl;
  enum extent_status{ATTR,CLEAN,DIRTY,DELETE};
  struct extent{
    std::string content;
    extent_protocol::attr attr;
    extent_status status;
  };
  std::map<extent_protocol::extentid_t, struct extent> extent_map;
 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf, int &);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 

