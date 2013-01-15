// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  pthread_mutex_init(&m,NULL);
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&m);
  if(extent_map.count(eid) == 0 || extent_map[eid].status == ATTR){
    printf("extent_client: file %lu do not exsit locally\n",eid);
    ret = cl->call(extent_protocol::get, eid, buf);
    extent_map[eid].content = buf;
    extent_map[eid].status = CLEAN;
    printf("extent_client: file %lu content is %s\n",eid, extent_map[eid].content.c_str());
  } else {
    if(extent_map[eid].status != DELETE){
      int t = time(NULL);
      printf("extent_client: file %lu exsit locally\n",eid);
      //ret = cl->call(extent_protocol::get, eid, buf);
      buf = extent_map[eid].content;
      extent_map[eid].attr.atime = t;
    } else {
      pthread_mutex_unlock(&m);
      return extent_protocol::NOENT;
    }
  }
  pthread_mutex_unlock(&m);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&m);
  if(extent_map.count(eid) == 0){
    ret = cl->call(extent_protocol::getattr, eid, attr);
    extent_map[eid].attr = attr;
    extent_map[eid].status = ATTR;
  } else {
    if(extent_map[eid].status != DELETE){
      attr = extent_map[eid].attr;
    } else {
      pthread_mutex_unlock(&m);
      return extent_protocol::NOENT;
    }
  }
  pthread_mutex_unlock(&m);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf,int &)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&m);
  printf("extent_client:put %llu\n",eid);
  int t = time(NULL);
  extent_map[eid].content = buf;
  extent_map[eid].attr.mtime = t;
  extent_map[eid].attr.ctime = t;
  extent_map[eid].attr.size = buf.size();
  extent_map[eid].status = DIRTY;
  pthread_mutex_unlock(&m);
  //int r;
  //ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  printf("extent_client:remove %llu\n",eid);
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&m);
  if(extent_map.count(eid) != 0){
    printf("extent_client:remove %llu exist\n",eid);
    extent_map[eid].status = DELETE;
  }
  //extent_map.erase(eid);
  pthread_mutex_unlock(&m);
  //int r;
  //ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}

extent_protocol::status extent_client::flush(extent_protocol::extentid_t eid){
  printf("extent_client: flush %llu is revoked\n", eid);
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&m);
  std::map<extent_protocol::extentid_t, struct extent>::iterator iter = extent_map.find(eid);
  if(iter != extent_map.end()){
    printf("extent_client: file %llu exist\n", iter->first);
    if(iter->second.status == DIRTY){
      printf("extent_client: file %llu is dirty and content is %s\n", iter->first, iter->second.content.c_str());
      int r;
      ret = cl->call(extent_protocol::put, iter->first, iter->second.content, r);
    } else if(iter->second.status == DELETE) {
      printf("extent_client: file %llu is deleted\n", iter->first);
      int r;
      ret = cl->call(extent_protocol::remove, iter->first, r);
    }
    extent_map.erase(iter->first);
  }
  pthread_mutex_unlock(&m);
  return ret;
}
