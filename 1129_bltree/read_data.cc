#include "bltree.h"
#include "MT.h"
#include <sys/time.h>

int 
main(int argc, char *argv[])
{
  if (argc != 2) {
    std::cout << "usage: program obj_num" << std::endl;
    exit(1);
  }
	int nobj = atoi(argv[1]);
	int *buf = (int *)calloc(nobj, sizeof(int)); if (!buf) ERR;
	int fd = open(DATA_FILE, O_RDONLY); if (fd == -1) ERR;
	int ret = read(fd, buf, sizeof(int) * nobj); if (ret == -1) ERR;
	close(fd);
  
  for (int i = 0; i < nobj; i++) {
    printf("%d ", buf[i]);
  }
  std::cout << std::endl;

  return 0;
}
