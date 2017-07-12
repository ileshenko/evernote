#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define GREP_LINE "cpu MHz"
int main(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *start;
	float mhz_f;
	uint64_t mhz_ul;

	fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1)
	{
		if (strncmp(line, GREP_LINE, strlen(GREP_LINE)))
			continue;
/*		printf("Retrieved line of length %ld :\n", read); */
		printf("%s", line);
		start = strchr(line, ':');
		sscanf(start + 1, "%f", &mhz_f);
		mhz_ul = mhz_f * 1000 * 1000;
		printf("===== %.1f === %lu\n", mhz_f, mhz_ul);
	}

	fclose(fp);
	free(line);
	exit(EXIT_SUCCESS);
}
