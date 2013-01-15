// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <iostream>
#include <pthread.h>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

struct lock {
  bool stat;
  pthread_cond_t cond;
};

std::map<lock_protocol::lockid_t, lock> locks;

pthread_mutex_t m;

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&m,NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  pthread_mutex_lock(&m);
  if(locks.count(lid) == 0){
    std::cout << "\nnew lock " << lid << std::endl;
    lock l;
    l.stat = false;
    pthread_cond_init(&(l.cond),NULL);
    locks.insert(std::map<lock_protocol::lockid_t, lock>::value_type(lid,l));
  } else {
    //printf("\nApply old lock %d\n", lid);
    while(locks[lid].stat==false){
	//printf("begin wait...\n");
      pthread_cond_wait(&(locks[lid].cond),&m);
	//printf("end wait...\n");
    }
    locks[lid].stat = false;
  }
  pthread_mutex_unlock(&m);
  r = 1;
  lock_protocol::status ret = lock_protocol::OK;
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  pthread_mutex_lock(&m);
  if(locks.count(lid)==1){
    locks[lid].stat = true;
    pthread_cond_signal(&(locks[lid].cond));
  }
  pthread_mutex_unlock(&m);
  r = 1;
  lock_protocol::status ret = lock_protocol::OK;
  //printf("\nRelease %d\n",lid);
  return ret;
}
