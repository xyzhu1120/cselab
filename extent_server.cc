// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

extent_server::extent_server() {
  int t = time(NULL);
  extent_protocol::attr a;
  a.atime = t;
  a.ctime = t;
  a.mtime = t;
  a.size = 0;
  inodes[0x00000001] = a;
  files[0x00000001] = "";
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &re)
{
  // You fill this in for Lab 2.
  printf("put %d",id);
  int t = time(NULL);
  if(inodes.count(id) == 0){
    extent_protocol::attr a;
    a.atime = t;
    a.mtime = t;
    a.ctime = t;
    a.size = buf.size();
    inodes[id]=a;
  } else {
    inodes[id].mtime = t;
    inodes[id].ctime = t;
    inodes[id].size = buf.size();
    //return extent_protocol::IOERR;
  }
  files[id]= buf;
  std::cout << files[id]<< std::endl << "Put OK" << std::endl;
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
// You fill this in for Lab 2.
  printf("get %d ", id);
  if(inodes.count(id) != 0){
    //printf("return %s", files[id]);
    int t = time(NULL);
    inodes[id].atime = t;
    buf = files[id];
  }
  else {
    printf("**not exist\n");
    return extent_protocol::NOENT;
  }
  std::cout << buf <<std::endl;
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  if(inodes.count(id) != 0 ){
    extent_protocol::attr tmp = inodes[id];
    a.size = tmp.size;
    a.atime = tmp.atime;
    a.mtime = tmp.mtime;
    a.ctime = tmp.ctime;
    return extent_protocol::OK;
  } else {
    return extent_protocol::IOERR;
  }
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  // You fill this in for Lab 2.
  std::map<extent_protocol::extentid_t, extent_protocol::attr>::iterator it;
  if( (it=inodes.find(id)) != inodes.end()){
    inodes.erase(it);
    files.erase(files.find(id));
    return extent_protocol::OK;
  } else {
  return extent_protocol::IOERR;
  }
}

