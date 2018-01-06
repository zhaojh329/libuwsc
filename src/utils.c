#include "utils.h"

int get_nonce(uint8_t *dest, int len)
{
	FILE *fp;
	size_t n;

	fp = fopen("/dev/urandom", "r");
	if (fp) {
		n = fread(dest, len, 1, fp);
		fclose(fp);
		return n;
	}

    return -1;
}

