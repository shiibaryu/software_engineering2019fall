#include "bltree.h"
#include "MT.h"
#include <sys/time.h>

typedef struct _HASH {
	int key;
	struct _HASH *next;
} HASH;

int
main(int argc, char* argv[])
{
	uint *buf;
	struct timeval begin, end;
	uint max_data = MAX_DATA;
	uint key_max;
	
	if (argc != 2) {
		std::cout << "Usage: program max_data" << std::endl;
		exit(1);
	}
	max_data = atoi(argv[1]);
	key_max = 2 * max_data;
	buf = (uint *)calloc(max_data, sizeof(uint)); if (!buf) ERR;

	std::cout << "max_data: " << max_data << std::endl;
	std::cout << "key_max: " << key_max << std::endl;
	
	gettimeofday(&begin, NULL);
	for (uint i = 0; i < max_data; i++) {
		if (i % 100000 == 0) DDD(i);
		while (true) {
			uint key = (uint)rand() % key_max;
			uint bid = key % max_data;
			if (key != 0 && buf[bid] == 0) {
				buf[bid] = key;
				break;
			}
		}
	}
	gettimeofday(&end, NULL);
	
	int fd = open(DATA_FILE, O_CREAT|O_WRONLY|O_APPEND|O_TRUNC, 0644); if (fd == -1) ERR;
	int ret = write(fd, buf, sizeof(int) * max_data); if (ret == -1) ERR;
	close(fd);
	free(buf);
	
	return 0;
}

