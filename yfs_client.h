#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client_cache.h"

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  lock_client *lc;
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
  int create(inum,const char *,inum &);
  int unlink(inum,const char *);
  int mkdir(inum,const char *,inum &);
  int setattr(inum,unsigned int);
  bool lookup(inum parent,const char *name,inum &);
  std::string read(inum,unsigned int,unsigned int);
  int write(inum,const char*,unsigned int, unsigned int);
  std::vector<struct dirent> readdir(inum);

  void flush(extent_protocol::extentid_t eid);
};

class my_lock_release_user:public lock_release_user {
public:
  yfs_client *yc;
  virtual void dorelease(lock_protocol::lockid_t);
  virtual ~my_lock_release_user() {};
};
#endif 
