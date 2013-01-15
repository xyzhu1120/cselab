// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  my_lock_release_user *mlru = new my_lock_release_user();
  mlru->yc = this;
  lc = new lock_client_cache(lock_dst,mlru);
  //srand((unsigned)time(NULL));
  srand((unsigned)getpid());
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::stringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::stringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  lc->acquire(inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    std::cout << "IO ERRORRR" << std::endl;
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:
  lc->release(inum);
  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock
  lc->acquire(inum);
  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    std::cout << "IO ERRORRR" << std::endl;
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  lc->release(inum);
  return r;
}

int
yfs_client::create(inum parent,const char *name, inum &id)
{
  printf("** Create parent %d\n", parent);
  int r = OK;
  int pos = 0;
  int oldpos = 0;
  std::string file(name);
  std::string name_s;
  int re;
  extent_protocol::attr a;
  inum i =  rand();
  i = i|0x80000000;
  while(ec->getattr(i,a) == extent_protocol::OK){
    i = rand();
    i = i|0x80000000;
  }
  std::string parentfolder;
  printf("**begin to get parent\n");
  lc->acquire(parent);
  if(ec->get(parent,parentfolder) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  //printf(" content of parent folder %s\n", parentfolder);
  //std::cout << "content is " << parentfolder << std::endl;
  while((pos = parentfolder.find('\n',oldpos)) != std::string::npos){
    name_s = parentfolder.substr(oldpos, pos - oldpos);
    if(name_s == file){
      printf("**Find EXIST\n");
      r = EXIST;
      goto release;
    }
    oldpos = parentfolder.find('\n',pos + 1) + 1;
  }
  parentfolder = parentfolder + file + '\n' + filename(i) + '\n';
  if ((ec->put(parent, parentfolder, re)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  lc->acquire(i);
  if ((ec->put(i, "",re)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  id = i;
release:
  lc->release(i);
  lc->release(parent);
  std::cout << "r" << r << std::endl;
  return r;
}

int
yfs_client::mkdir(inum parent,const char *name, inum &id)
{
  printf("** Create parent %d\n", parent);
  int r = OK;
  int pos = 0;
  int oldpos = 0;
  std::string file(name);
  std::string name_s;
  int re;
  extent_protocol::attr a;
  inum i =  rand();
  i = i&0x7fffffff;
  while(ec->getattr(i,a) == extent_protocol::OK){
    i = rand();
    i = i&0x7fffffff;
  }
  std::string parentfolder;
  printf("**begin to get parent\n");
  lc->acquire(parent);
  if(ec->get(parent,parentfolder) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  //printf(" content of parent folder %s\n", parentfolder);
  //std::cout << "content is " << parentfolder << std::endl;
  while((pos = parentfolder.find('\n',oldpos)) != std::string::npos){
    name_s = parentfolder.substr(oldpos, pos - oldpos);
    if(name_s == file){
      printf("**Find EXIST\n");
      r = EXIST;
      goto release;
    }
    oldpos = parentfolder.find('\n',pos + 1) + 1;
  }
  parentfolder = parentfolder + file + '\n' + filename(i) + '\n';
  if ((ec->put(parent, parentfolder, re)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  lc->acquire(i);
  if ((ec->put(i, "",re)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  id = i;
release:
  lc->release(i);
  lc->release(parent);
  std::cout << "r" << r << std::endl;
  return r;
}

int yfs_client::unlink(inum parent, const char *name){
  printf("** Edit parent %d\n", parent);
  int r = OK;
  int pos = 0;
  bool find = false;
  int oldpos = 0;
  std::string file(name);
  std::string name_s;
  int re;
  std::string parentfolder;
  printf("**begin to get parent\n");
  lc->acquire(parent);
  if(ec->get(parent,parentfolder) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  //printf(" content of parent folder %s\n", parentfolder);
  //std::cout << "content is " << parentfolder << std::endl;
  inum id;
  while((pos = parentfolder.find('\n',oldpos)) != std::string::npos){
    name_s = parentfolder.substr(oldpos, pos - oldpos);
    if(name_s == file){
      printf("**Find EXIST\n");
      int tmppos= pos + 1;
      pos = parentfolder.find('\n',pos + 1);
      std::string inum_s = parentfolder.substr(tmppos, pos - tmppos);
      id = n2i(inum_s);
      find = true;
      break;
    }
    oldpos = parentfolder.find('\n',pos + 1) + 1;
  }
  if(!find){
    lc->release(parent);
    return ENOENT;
  }
  parentfolder.erase(oldpos,pos - oldpos + 1);
  if ((ec->put(parent, parentfolder, re)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  lc->acquire(id);
  if ((ec->remove(id)) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
release:
  lc->release(id);
  lc->release(parent);
  std::cout << "r" << r << std::endl;
  return r;
}

bool yfs_client::lookup(inum parent,const char *name,inum &id){
  printf("**Look up\n");
  std::string content;
  std::string filename(name);
  printf("**Look up get\n");
  lc->acquire(parent);
  if((ec->get( parent, content)) != extent_protocol::OK){
    printf("**Get operation failed\n");
    return false;
  }
  lc->release(parent);
  //printf("**content is %s",content);
  std::cout << content << std::endl;
  printf("**Look up\n");
  int pos = 0;
  int oldpos = 0;
  while((pos = content.find('\n',oldpos)) != std::string::npos){
    std::string name = content.substr(oldpos, pos - oldpos);
   // std::cout << name << filename << std::endl;
    if(name == filename){
      oldpos = pos + 1;
      pos = content.find('\n',oldpos);
      std::string inum_s = content.substr(oldpos, pos - oldpos);
      id = n2i(inum_s);
      return true;
    }
    oldpos = content.find('\n',pos + 1) + 1;
  }
  return false;
}

std::vector<yfs_client::dirent> yfs_client::readdir(inum parent){
  printf("**Readdir\n");
  std::vector<struct dirent> ents;
  std::string content;
  lc->acquire(parent);
  ec->get(parent, content);
  lc->release(parent);
  std::cout << "content:" << content << std::endl;
  int pos = 0;
  int oldpos = 0;
  while((pos = content.find('\n',oldpos)) != std::string::npos){
    std::string name = content.substr(oldpos, pos - oldpos);
    oldpos = pos + 1;
    pos = content.find('\n',oldpos) ;
    std::string inums = content.substr(oldpos, pos - oldpos);
    dirent ent;
    ent.name = name;
    ent.inum = n2i(inums);
    std::cout << name << " " << inums << std::endl;
    ents.push_back(ent);
    oldpos = pos + 1;
  }
  return ents;
}

int yfs_client::setattr(inum i, unsigned int size)
{
  printf("**setattr\n");
  std::string content;
  std::string result;
  lc->acquire(i);
  ec->get(i,content);
  if(size <= content.size()){
    result = content.substr(0,size);
  } else {
    result = content;
    int offset = size - content.size();
    for(int cnt = 0; cnt < offset; ++cnt) {
      result = result + '\0';
    }
  }
  int r;
  ec->put(i,result,r);
  lc->release(i);
  return 1;
} 

std::string yfs_client::read(inum i, unsigned int size, unsigned int offset)
{
  std::string content;
  std::cout << "**Read " << size << " " << offset << std::endl;
  lc->acquire(i);
  ec->get(i,content);
  lc->release(i);
  std::cout << "**content " << content << std::endl;
  if(offset >= content.size())
    return "";
  if(size+offset > content.size()){
    size = content.size() - offset;
    std::cout << "**return size" << offset << " " << size << std::endl;
  }
  std::cout << "**return str " << content.substr(offset, size) << std::endl;
  return content.substr(offset, size);
}

int yfs_client::write(inum i,const char* buf, unsigned int size, unsigned int offset)
{
  std::cout << "**Write " << size << " " << offset << " " << buf << std::endl;
  std::string content;
  std::string result;
  lc->acquire(i);
  ec->get(i, content);
  std::cout << "**content " << content << std::endl;
  if(offset > content.size()){
    for(int cnt = content.size(); cnt < offset; ++cnt) {
      content = content + '\0';
    }
  }
  std::string buf_s(buf,size);
  result = content.substr(0,offset) + buf_s.substr(0,size);
  if(offset + size < content.size()){
    result = result + content.substr(offset+size);
  }
  int r;
  ec->put(i, result, r);
  lc->release(i);
  return 1;
}
  
void yfs_client::flush( extent_protocol::extentid_t eid){
  this->ec->flush(eid);
}

void  my_lock_release_user::dorelease(lock_protocol::lockid_t lid) {
  this->yc->flush(lid);
}
