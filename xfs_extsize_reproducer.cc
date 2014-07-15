/**
 * Try changing extsize on a non-empty sparse object
 */

#include <xfs/xfs.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define OBJSIZE (4<<20)
#define ATTRSIZEMAX (1<<10)

int main(int argc, char **argv) {
  int fd = open("test", O_RDWR|O_CREAT|O_EXCL, 0666);
  assert(fd >= 0);
  int r = 0;

  if (argc >= 2 && (argv[1][0] == 'e' || argv[1][0] == 'a')) {
    struct fsxattr fsx;
    r = ioctl(fd, XFS_IOC_FSGETXATTR, &fsx);
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
  } else {
    std::cerr << "did not set extsize" << std::endl;
  }

  char *buf = new char[OBJSIZE];
  memset(buf, 0, OBJSIZE);
  int offset = -1;
  int len = -1;
  while (scanf("%d %d\n", &offset, &len) == 2) {
    assert(len >= 0);
    assert(offset >= 0);
    std::cout << offset << "~" << len << std::endl;
    assert(len <= OBJSIZE);
    assert(offset + len <= OBJSIZE);

    r = pwrite(fd, buf, len, offset);
    assert(r == len);

    if (argc >= 2 && (argv[1][0] == 'f' || argv[1][0] == 'a')) {
      std::cerr << "fadvising" << std::endl;
      r = posix_fadvise(fd, offset, len, POSIX_FADV_DONTNEED);
      assert(r == 0);
    }
    fsync(fd);
    close(fd);

    char *check = new char[OBJSIZE];

    fd = open("test", O_RDWR, 0666);
    assert(fd >= 0);
    r = pread(fd, check, OBJSIZE, 0);

    r = memcmp(buf, check, r);
    assert(r == 0);
    delete[] check;
  }

  delete[] buf;
  close(fd);
}
