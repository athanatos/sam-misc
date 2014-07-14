/**
 * Try changing extsize on a non-empty sparse object
 */

#include <xfs/xfs.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <string.h>

#define OBJSIZE (4<<20)

int main() {
  int randfd = open("/dev/urandom", O_RDONLY);
  int fd = open("test", O_RDWR|O_CREAT|O_EXCL, 0666);
  assert(fd >= 0);

  struct fsxattr fsx;
  int r = ioctl(fd, XFS_IOC_FSGETXATTR, &fsx);
  if (r < 0) {
    int ret = -errno;
    std::cerr << "FSGETXATTR: " << ret << std::endl;
    return ret;
  }

  // already set?
  if (fsx.fsx_xflags & XFS_XFLAG_EXTSIZE) {
    std::cerr << "already set" << std::endl;
    return 0;
  }

  std::cerr << fsx.fsx_nextents << " exents, extsize is " << fsx.fsx_extsize << std::endl;

  unsigned val = 4<<20;
  fsx.fsx_xflags |= XFS_XFLAG_EXTSIZE;
  fsx.fsx_extsize = val;
  
  if (ioctl(fd, XFS_IOC_FSSETXATTR, &fsx) < 0) {
    int ret = -errno;
    std::cerr << "FSSETXATTR: " << ret << std::endl;
    return ret;
  }

  struct fsxattr fsx2;
  r = ioctl(fd, XFS_IOC_FSGETXATTR, &fsx2);
  if (r < 0) {
    int ret = -errno;
    std::cerr << "FSGETXATTR: " << ret << std::endl;
    return ret;
  }

  if (fsx2.fsx_xflags & XFS_XFLAG_EXTSIZE) {
    std::cerr << "successfully set to " << fsx2.fsx_extsize << std::endl;
  }

  char *buf = new char[OBJSIZE];
  char *check = new char[OBJSIZE];
  memset(check, 0, OBJSIZE);
  memset(buf, 0, OBJSIZE);
  int offset = -1;
  int len = -1;
  while (scanf("%d %d\n", &offset, &len) == 2) {
    assert(len >= 0);
    assert(offset >= 0);
    std::cout << offset << "~" << len << std::endl;
    assert(len <= OBJSIZE);
    assert(offset + len <= OBJSIZE);

    r = read(randfd, buf, len);
    assert(r == len);
    memcpy(check + offset, buf, len);
    r = pwrite(fd, buf, len, offset);
    assert(r == len);

    if (rand() % 3 == 0)
      fsync(fd);
    if (rand() % 3 == 0)
      syncfs(fd);
    close(fd);
    fd = open("test", O_RDWR, 0666);
    assert(fd >= 0);
  }

  r = pread(fd, buf, OBJSIZE, 0);
  assert(r == OBJSIZE);
  r = memcmp(buf, check, OBJSIZE);
  assert(r == 0);

  delete[] buf;
  delete[] check;
  close(fd);
  close(randfd);
}
