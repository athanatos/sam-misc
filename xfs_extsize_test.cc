/**
 * Try changing extsize on a non-empty sparse object
 */

#include <xfs/xfs.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <string.h>

int main() {
  char buf[256];
  memset(buf, 1, sizeof(buf));

  int fd = open("test", O_RDWR|O_CREAT|O_EXCL, 0666);
  assert(fd >= 0);

  int r = pwrite(fd, buf, 1, 24<<10);
  assert(r == 1);

  close(fd);

  fd = open("test", O_RDWR);
  assert(fd >= 0);

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

  close(fd);

#if 0
  fd = open("test", O_RDONLY);
  assert(fd >= 0);

  char zbuf[24<<10];
  memset(zbuf, 0, sizeof(zbuf));
  char obuf[24<<10];
  r = read(fd, obuf, sizeof(obuf));
  assert(r == sizeof(obuf));

  r = memcmp(zbuf, obuf, sizeof(zbuf));
  assert(r == 0);
  close(fd);
#endif
}
