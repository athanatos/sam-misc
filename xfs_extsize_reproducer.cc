/**
 * Test extsize behavior
 */

#include <xfs/xfs.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define OBJSIZE (4<<20)

void drop_caches() {
  int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
  assert(fd >= 0);
  int r = write(fd, "3", 1);
  assert(r == 1);
}

int main(int argc, char **argv) {
  int fd = open("test", O_RDWR|O_CREAT|O_EXCL, 0666);
  int randfd = open("/dev/urandom", O_RDONLY);
  assert(fd >= 0);
  int r = 0;

  if (argc >= 2 && (strchr(argv[1], 'e') != NULL)) {
    struct fsxattr fsx;
    r = ioctl(fd, XFS_IOC_FSGETXATTR, &fsx);
    if (r < 0) {
      int ret = -errno;
      std::cout << "FSGETXATTR: " << ret << std::endl;
      return ret;
    }

    // already set?
    if (fsx.fsx_xflags & XFS_XFLAG_EXTSIZE) {
      std::cout << "already set" << std::endl;
      return 0;
    }

    std::cout << fsx.fsx_nextents << " exents, extsize is " << fsx.fsx_extsize << std::endl;

    unsigned val = 4<<20;
    fsx.fsx_xflags |= XFS_XFLAG_EXTSIZE;
    fsx.fsx_extsize = val;
  
    if (ioctl(fd, XFS_IOC_FSSETXATTR, &fsx) < 0) {
      int ret = -errno;
      std::cout << "FSSETXATTR: " << ret << std::endl;
      return ret;
    }

    struct fsxattr fsx2;
    r = ioctl(fd, XFS_IOC_FSGETXATTR, &fsx2);
    if (r < 0) {
      int ret = -errno;
      std::cout << "FSGETXATTR: " << ret << std::endl;
      return ret;
    }

    if (fsx2.fsx_xflags & XFS_XFLAG_EXTSIZE) {
      std::cout << "successfully set to " << fsx2.fsx_extsize << std::endl;
    }
  } else {
    std::cout << "did not set extsize" << std::endl;
  }

  char *buf = new char[OBJSIZE];
  char *check = new char[OBJSIZE];
  memset(buf, 0, OBJSIZE);
  memset(check, 0, OBJSIZE);
  int offset = -1;
  int len = -1;
  while (scanf("%d %d\n", &offset, &len) == 2) {
    assert(len >= 0);
    assert(offset >= 0);
    assert(len <= OBJSIZE);
    assert(offset + len <= OBJSIZE);

    std::cout << offset << "~" << len;
    if (argc >= 2 && (strchr(argv[1], 'r') != NULL)) {
      std::cout << " - random" << std::endl;
      read(randfd, buf, len);
    } else {
      std::cout << " - zero" << std::endl;
      memset(buf, 0, len);
    }

    r = pwrite(fd, buf, len, offset);
    assert(r == len);
    memcpy(check + offset, buf, len);

    if (argc >= 2 && (strchr(argv[1], 'f') != NULL)) {
      std::cout << "fadvising" << std::endl;
      r = posix_fadvise(fd, offset, len, POSIX_FADV_DONTNEED);
      assert(r == 0);
    }
    close(fd);

    if (argc >= 2 && (strchr(argv[1], 'd') != NULL)) {
      drop_caches();
    }

    fd = open("test", O_RDWR, 0666);
    assert(fd >= 0);
    int len = pread(fd, buf, OBJSIZE, 0);
    assert(len >= 0);

    r = memcmp(buf, check, len);
    if (r != 0) {
      std::cout << "bufs don't match, outputting differing extents:" << std::endl;
      bool matching = buf[0] == check[0];
      int begin = 0;
      for (int i = 0; i < len; ++i) {
	if ((buf[i] == check[i]) ^ matching) {
	  if (!matching) {
	    std::cout << begin << "~" << i-begin-1 << " does not match" << std::endl;
	    matching = true;
	  } else {
	    matching = false;
	  }
	  begin = i;
	}
      }
      return 1;
    }
  }

  delete[] buf;
  close(randfd);
  close(fd);
}
