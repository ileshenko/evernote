#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[1;31m"
#define ANSI_COLOR_GREEN   "\x1b[1;32m"
#define ANSI_COLOR_YELLOW  "\x1b[1;33m"
#define ANSI_COLOR_BLUE    "\x1b[1;34m"
#define ANSI_COLOR_MAGENTA "\x1b[1;35m"
#define ANSI_COLOR_CYAN    "\x1b[1;36m"
#define ANSI_COLOR_RESET   "\x1b[22;37m"

#if 0
f1=open('/tmp/igorle_profile', 'w+')

def prnt():
	print >> f1, cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM',
	mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False,
	gci_copy=False, copy_index=0, length=4*4096).result.get('params').get('data');
	f1.flush()

imem_parse /tmp/igorle_profile 48 4096

print >> f1, cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False, gci_copy=False, copy_index=0, length=4*4096).result.get('params').get('data') ; f1.flush()

cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, range=True, range_step=1, single_copy=False, gci_copy=False, copy_index=0, data_size=1, data='00', range_size=2048)

cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False, gci_copy=False, copy_index=0, length=8).result.get('params').get('data')

#endif

static int print_list = 0;

static struct {
	uint32_t id, stage;
	char *comment;
} comments[] = {
	{0, 0x000, "Left classifier"},
	{128, 0x000, "Right classifier"},
	{253, 0x000, "Enc. lookup"},
	{253, 0x001, "Wait for sorter"},
	{253, 0x003, "sec slot alloc"},
	{253, 0x007, "HW encryption"},
	{253, 0x00f, "free slot and finish"},
	{254, 0x000, "Dec. lookup"},
	{254, 0x001, "sec slot alloc"},
	{254, 0x003, "HW decription"},
	{254, 0x007, "Wait for sorter"},
	{254, 0x00f, "Finish"},
	{0, 0, NULL}
};

static char *comment_get(uint32_t id, uint32_t stage)
{
	int i;

	for (i = 0; comments[i].comment; i++)
	{
		if (comments[i].id == id && comments[i].stage == stage)
			return comments[i].comment;
	}

	return NULL;
}

//#define MSK(val) (val)
void print_1(uint32_t *vals, int first, int last)
{
	printf("val %#010x: %d hits\n", vals[first], 1 + last - first);
}

#define MSK(val) ((val)&0xfffff000)
void print_2(uint32_t *vals, int first, int last)
{
	uint32_t id, stage;

	if (vals[first] == 0xffffffff)
	{
		print_1(vals, first, last);
		return;
	}

	id = vals[first] >> 24;
	if (id == 253)
		printf(ANSI_COLOR_CYAN);
	else if (id == 254)
		printf(ANSI_COLOR_BLUE);

	stage = vals[first] & 0x00fff000;
	stage >>= 12;

	printf("id %3d | %03x (%d): ", id, stage, 1 + last - first);
	if (comment_get(id, stage))
		printf("\t%s ", comment_get(id, stage));
	if (print_list)
	{
		for (; first <= last; first++)
			printf(" %d", vals[first] & 0x0fff);
	}

	printf(ANSI_COLOR_RESET "\n");
}

static void report(uint32_t *vals, int cnt)
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
		if (i == cnt-1 || MSK(vals[i]) != MSK(vals[i+1]))
		{
			print_2(vals, j, i);
//			printf("val %#010x: %d hits\n", vals[i], i+1-j);
			j = i+1;
		}
	}
}

static void histogram(uint32_t *vals)
{
	int i, j, idle, idles;

	idles = 0;
	for (i = 0; i < 256; i++)
	{
		idle = 0;
		for (j = 0; j<16; j++)
		{
			if (vals[i*16+j] == 0xffffffff)
				idle++;
		}
		idles += idle;
		printf("%3d", idle);
		if ((i&0xf) == 0xf)
			printf("\n");
	}
	printf("idles %d\n", idles);
}

static void classifiers(uint32_t *vals)
{
	int i, j, used, all;
	int id;

	all = 0;
	for (i = 0; i < 256; i++)
	{
		used = 0;
		for (j = 0; j<16; j++)
		{
			id = vals[i*16+j] >> 24;
			if (id == 0 || id == 128)
				used++;
		}
		all += used;
		printf("%3d", used);
		if ((i&0xf) == 0xf)
			printf("\n");
	}
	printf("all used %d\n", all);
}

static uint32_t get_int(char *bf)
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
	printf("ERROR: argc should be 4 or 5, not %d\n", argc);
	printf("Usage: %s <filename> <first val> <last val> [L]\n", argv[0]);
}

int main(int argc, char* argv[])
{
	FILE *f;
	char buff[256];
	uint32_t vals[4096];
	int start, stop;
	int tail, cnt, i;

	if (argc != 4 && argc != 5)
	{
		usage(argc, argv);
		return 1;
	}

	if (argc == 5)
		print_list = 1;

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
//				histogram(vals);
//				classifiers(vals);
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
