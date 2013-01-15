// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();
  pthread_mutex_init(&m,NULL);
  lru = lu;
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  tprintf("*** %s aquire %llu \n",this->id.c_str(), lid);
  int ret = lock_protocol::OK;
  int r;
  pthread_mutex_lock(&m);
  if(locks.count(lid) == 0){
    lock l;
    l.stat = 3;
    l.used = false;
    pthread_cond_init(&(l.cond),NULL);
    locks.insert(std::map<lock_protocol::lockid_t, lock>::value_type(lid, l));
    pthread_mutex_unlock(&m);
    ret = cl->call(lock_protocol::acquire,lid, this->id, r);
    if(ret == lock_protocol::OK){
      pthread_mutex_lock(&m);
      if(locks[lid].stat != 4){
        locks[lid].stat = 2;
      }
      locks[lid].used = true;
      pthread_mutex_unlock(&m);
      return lock_protocol::OK; 
    } else if(ret == lock_protocol::RPCERR)
        return ret;
    pthread_mutex_lock(&m);
  }
  tprintf("****going to wait %llu and lock stat is %i \n", lid, locks[lid].stat);
  while(locks[lid].stat != 1){
    if(locks[lid].stat == 0){
      tprintf("****goint to acquire lock remotely on client %s \n", this->id.c_str());
      locks[lid].stat = 3;
      pthread_mutex_unlock(&m);
      ret = cl->call(lock_protocol::acquire, lid, this->id, r);
      if(ret == lock_protocol::OK){
        pthread_mutex_lock(&m);
        if(locks[lid].stat != 4)
          locks[lid].stat = 2;
        locks[lid].used = true;
        pthread_mutex_unlock(&m);
        return lock_protocol::OK; 
      }else if(ret == lock_protocol::RPCERR)
        return ret;
      tprintf("*****on client %s acquire receive retry pthread %u****\n", this->id.c_str(), (unsigned int)pthread_self())
      pthread_mutex_lock(&m);
    } 
    if(locks[lid].stat == 4 && locks[lid].used == false){
      locks[lid].used = true;
      pthread_mutex_unlock(&m);
      return lock_protocol::OK;
    }
    if(locks[lid].stat == 1){
      break;
    } else if(locks[lid].stat == 0){
      continue;
    } else {
      tprintf("****\non client %s I will go to sleep stat %i %s\n",this->id.c_str(), locks[lid].stat,(locks[lid].used)?"used":"free");
      pthread_cond_wait(&(locks[lid].cond),&m);
    } 
    tprintf("lock %llu stat: %i %s\n on client %s", lid, locks[lid].stat, (locks[lid].used)?"true":"false", this->id.c_str());
    tprintf("****wake up on %s \n", this->id.c_str());
  }
  locks[lid].stat = 2;
  locks[lid].used = true;
  pthread_mutex_unlock(&m);
  tprintf("**** client %s acquire %llu succeed\n", this->id.c_str(), lid);
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  tprintf("***release %llu \n", lid);
  int ret = lock_protocol::OK;
  pthread_mutex_lock(&m);
  if(locks.count(lid) != 0){
    int r;
    tprintf("****lock stat %i\n", locks[lid].stat);
    if(locks[lid].stat==4) {
      tprintf("****remote release %llu \n", lid);
      lu->dorelease(lid);
      pthread_mutex_unlock(&m);
      cl->call(lock_protocol::release, lid, this->id, r);
      pthread_mutex_lock(&m);
      locks[lid].stat = 0;
      locks[lid].used = false;
    } else {
      tprintf("****local release %llu \n", lid);
      locks[lid].stat = 1;
    }
    pthread_cond_signal(&(locks[lid].cond));
  }
  pthread_mutex_unlock(&m);
  return lock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &r)
{
  pthread_mutex_lock(&m);
  tprintf("***revoke %llu at %s stat %i %s\n",lid, this->id.c_str(),locks[lid].stat, (locks[lid].used)?"used":"no");
  if(locks.count(lid) != 0){
    if(locks[lid].stat == 0){
      pthread_mutex_unlock(&m);
      return rlock_protocol::OK;
    }
    if(locks[lid].stat == 1 && locks[lid].used == true){
      /*
      locks[lid].stat = 0;
      r = 1;
      tprintf("****directly revoke\n");
      */ 
      //tprintf("\n*****\n%s will call the release of server at revole\n****\n", this->id.c_str());
      locks[lid].stat = 0;
      locks[lid].used = false;
      lu->dorelease(lid);
      int r;
      cl->call(lock_protocol::release, lid, this->id, r);
      pthread_mutex_unlock(&m);
      //tprintf("\n*****\n%s finish call the release of server at revole\n****\n", this->id.c_str());
      return rlock_protocol::OK;
    } else {
      locks[lid].stat = 4;
      r = 0;
    }
  }
  pthread_mutex_unlock(&m);
  int ret = rlock_protocol::OK;
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;
  pthread_mutex_lock(&m);
  tprintf("***RETRY %llu at %s\n stat %i\n",lid, this->id.c_str(), locks[lid].stat);
  if(locks.count(lid) != 0){
    if(locks[lid].stat != 4)
    	locks[lid].stat = 1;
    pthread_cond_signal(&(locks[lid].cond));
  }
  pthread_mutex_unlock(&m);
  tprintf("****RETRY SUCCEED %llu \n",lid);
  return ret;
}
