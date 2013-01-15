// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"
#include <queue>
#include <pthread.h>



lock_server_cache::lock_server_cache()
{
  pthread_mutex_init(&m,NULL);
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &r)
{
  tprintf("***client %s acquire lock %llu \n",id.c_str(),lid);
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&m);
  if(locks.count(lid) == 0){
    tprintf("****new lock\n");
    lock l;
    l.stat = false;
    l.id = id;
    locks.insert(std::map<lock_protocol::lockid_t, lock>::value_type(lid,l));
    r = 1;
    pthread_mutex_unlock(&m);
  } else {
    if(locks[lid].stat == false){
      tprintf("****lock is occupied client %s has to wait lock %llu \n",id.c_str(),lid);

      locks[lid].q.push(id);
      if(locks[lid].q.size() > 1){
        tprintf("***\ndo not need to revoke\n***\n");
        pthread_mutex_unlock(&m);
        return lock_protocol::RETRY;
      }
        std::string holder = locks[lid].id;
  pthread_mutex_unlock(&m);
        handle h(holder);
        rpcc *cl = h.safebind();
       lock_protocol::status ret2;
      tprintf("****revoke %s", holder.c_str());
      int a = 0;
      if(cl){
        ret2 = cl->call(rlock_protocol::revoke, lid, a);
        tprintf("return %i a\n",a);
      } else {
        return lock_protocol::RPCERR;
      }
      tprintf("****\n%sacquire succeed retry\n***\n",id.c_str());
      return lock_protocol::RETRY; 
/*
      locks[lid].q.push(id);
      while(locks[lid].q.push(id).size() > 0){
        std::string holder = locks[lid].id;
        std::string nextholder = locks[lid].q.front();
        locks[lid].q.pop();
        pthread_mutex_unlock(&m);
        handle h(holder);
        rpcc *cl = h.safebind();
        int a = 0;
      lock_protocol::status ret2;
      tprintf("****revoke %s", holder.c_str());
      if(cl){
        ret2 = cl->call(rlock_protocol::revoke, lid, a);
        tprintf("return %i a\n",a);
      } else {
        return lock_protocol::RPCERR;
      }
      if(a == 0)
        break;
      else{
        pthread_mutex_lock(&m);
        handle h(holder);
        rpcc *cl = h.safebind();
        if(cl){
          cl->call(rlock_protocol::retry, lid, a);
        } else return lock_protocol::RPCERR;
        
      }
      pthread_mutex_lock(&m);
      }
*/
      /*
      tprintf("****client %s has to wait lock %llu \n",id.c_str(),lid);
      std::string holder = locks[lid].id;
      ret = lock_protocol::RETRY;
      pthread_mutex_unlock(&m);
      handle h(holder);
      rpcc *cl= h.safebind();
      int a = 0;
      lock_protocol::status ret2;
      tprintf("****revoke %s", holder.c_str());
      if(cl){
        ret2 = cl->call(rlock_protocol::revoke, lid, a);
        tprintf("return %i a\n",a);
      } else {
        return lock_protocol::RPCERR;
      }
      if(a == 1){
        tprintf("****client %s lock %llu xidangdie size %i\n",id.c_str(),lid, locks[lid].q.size());
        pthread_mutex_lock(&m);
        locks[lid].stat = false;
        locks[lid].id = id;
        if(locks[lid].q.size() > 0){
          pthread_mutex_unlock(&m);
          handle h(id);
          rpcc *cl= h.safebind();
          if(cl){
            ret2 = cl->call(rlock_protocol::revoke, lid, a);
          } else {
            return lock_protocol::RPCERR;
          }
        } 
        pthread_mutex_unlock(&m);
        return lock_protocol::OK;
      }
      locks[lid].q.push(id);
      tprintf("****queue grow %i \n",locks[lid].q.size());
*/
    }else{
      locks[lid].stat = false;
      locks[lid].id = id;
      pthread_mutex_unlock(&m);
    }  
  }
  tprintf("***%s*acquire succeed\n",id.c_str());
  return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  tprintf("***client %s release lock %llu \n",id.c_str(),lid);
  pthread_mutex_lock(&m);
  if(locks.count(lid) != 0){
    if(locks[lid].q.empty()){
      locks[lid].stat = true;
      pthread_mutex_unlock(&m);
    }else{
      //while(!locks[lid].q.empty()){
      std::string nextowner = locks[lid].q.front();
      locks[lid].q.pop();
      locks[lid].id = nextowner;
      tprintf("****lock %llu has a queue size %i and next is %s \n",lid,locks[lid].q.size(), nextowner.c_str());
      pthread_mutex_unlock(&m);
      handle h(nextowner);
      rpcc *cl = h.safebind();
      int a = 0;
      if(cl){
        cl->call(rlock_protocol::retry, lid, a);
        pthread_mutex_lock(&m);
        if(!locks[lid].q.empty()){
          tprintf("****lock %llu 's queue is long\n",lid);
          pthread_mutex_unlock(&m);
       	  cl->call(rlock_protocol::revoke, lid,a);
        }
      } else {
        return lock_protocol::RPCERR;
      }
      //if(a == 0){
      //  break;
      //}
      pthread_mutex_unlock(&m);
    }
  }
  lock_protocol::status ret = lock_protocol::OK;
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

