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

int parse_url(const char *url, char **host, int *port, const char **path)
{
    char *p;
    const char *host_pos;
    int host_len = 0;

    if (strncmp(url, "ws://", 5))
        return -1;
    
    url += 5;
    host_pos = url;

    p = strchr(url, ':');
    if (p) {
        host_len = p - url;
        url = p + 1;
        *port = atoi(url);
    }

    p = strchr(url, '/');
    if (p) {
        *path = p;
        if (host_len == 0)
            host_len = p - host_pos;
    }

    if (host_len == 0)
        host_len = strlen(host_pos);

    *host = strndup(host_pos, host_len);

    return 0;
}

