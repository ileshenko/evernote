#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if 0
f1=open('/tmp/testfile', 'w+')

print >> f1, cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False, gci_copy=False, copy_index=0, length=2048).result.get('params').get('data') ; f1.flush()

./parse /tmp/testfile 48 512

cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, range=True, range_step=1, single_copy=False, gci_copy=False, copy_index=0, data_size=1, data='00', range_size=2048)

#endif

static void report(int *vals, int cnt)
{
	unsigned int i, j, tmp;

	for (i=0; i<cnt-1; i++)
	{
		for (j=i+1; j<cnt; j++)
		{
			if (vals[j] < vals[i])
			{
				tmp = vals[i];
				vals[i] = vals[j];
				vals[j] = tmp;
			}
		}
	}

	printf("=============================\n");
	for (i=0, j=0; i<cnt; i++)
	{
		if (i == cnt-1 || vals[i] != vals[i+1])
		{
			printf("val %#010x: %d hits\n", vals[i], i+1-j);
			j = i+1;
		}
	}
}

static int get_int(char *bf)
{
	uint32_t a,b,c,d;

	if (sscanf(bf, "%x %x %x %x", &a, &b, &c, &d) != 4)
		exit(5);
	return a << 24 | b << 16 | c << 8 | d;
}

static int int_completed(char *b)
{
	int i;

	for (i = 0; i < 11; i++)
	{
		if (!b[i] || b[i] == '\n')
			return 0;
	}
	return 1;
}

static int buff_move(char *b, int used)
{
	int i = 0;

	memmove(b, b+used, 256-used);

	while (b[i] && b[i] != '\n')
		i++;

	return i;
}

static void usage(int argc, char* argv[])
{
	printf("ERROR: argc should be 4, not %d\n", argc);
	printf("Usage: %s <filename> <first val> <last val>\n", argv[0]);
}

int main(int argc, char* argv[])
{
	FILE *f;
	char buff[256];
	int vals[4096];
	int start, stop;
	int tail, cnt, i;

	if (argc != 4)
	{
		usage(argc, argv);
		return 1;
	}

	f = fopen(argv[1], "r+t");

	if (!f)
	{
		fprintf(stderr, "Can't open file \"%s\": ", argv[1]);
		perror(NULL);
		return 2;
	}

	start = atoi(argv[2]);
	stop = atoi(argv[3]);

	tail = 0;
	cnt = 0;
	for (;;)
	{
		if (!fgets(&buff[tail], 256-tail, f))
		{
			sleep(1);
			continue;
		}

		for (i = 0;; i+=11)
		{
			if (buff[i] == ' ')
				i++;

			if (buff[i] == '\n')
			{
				report(&vals[start], stop - start);
				cnt = 0;
				tail = 0;
				break;
			}

			if (int_completed(buff+i))
			{
				vals[cnt++] = get_int(buff+i);
			}
			else
			{
				tail = buff_move(buff, i);
				break;
			}
		}
	}

	fclose(f);

	return 0;
}
