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

/* python commands:
 
f1=open('/tmp/igorle_chaos', 'w+')

def prnt():
	print >> f1, cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM',
	mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False,
	gci_copy=False, copy_index=0, length=4*1024 + 4 + 4).result.get('params').get('data');
	f1.flush()

def rst():
	cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0,
	lsb_address=0, msb_address=0, range=True, range_step=1, single_copy=False,
	gci_copy=False, copy_index=0, data_size=1, data='00', range_size=4096)

def stp(shift):
	cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0,
	lsb_address=1024*4+4+3, msb_address=0, range=True, range_step=1,
	single_copy=False, gci_copy=False, copy_index=0, data_size=1, data=shift,
	range_size=1)

def sw(mode):
	cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0,
	lsb_address=1024*4+3, msb_address=0, range=True, range_step=1,
	single_copy=False, gci_copy=False, copy_index=0, data_size=1, data=mode,
	range_size=1)

*/

#if 0
ichaos_parse /tmp/igorle_chaos

rst()
prnt()
stp('0a')

#endif

#if 0
print >> f1, cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False, gci_copy=False, copy_index=0, length=4*1027).result.get('params').get('data') ; f1.flush()

def sw(mode):
	cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=1024*4+8+3, msb_address=0, range=True, range_step=1, single_copy=False, gci_copy=False, copy_index=0, data_size=1, data=mode, range_size=1)


Set shift
cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=1024*4+4+3, msb_address=0, range=True, range_step=1, single_copy=False, gci_copy=False, copy_index=0, data_size=1, data='09', range_size=1)

Clear
cpe.cp.prm.write_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, range=True, range_step=1, single_copy=False, gci_copy=False, copy_index=0, data_size=1, data='00', range_size=4096)

cpe.cp.prm.read_mem(channel_id=0, mem_id='INT_MEM', mem_space_index=0, lsb_address=0, msb_address=0, single_copy=False, gci_copy=False, copy_index=0, length=8).result.get('params').get('data')
#endif

static void print_chart(int *chart)
{
	int i, p;
	int line_is_blank;
	char *clr, chr;

	for (p = 32; p > 0; p--)
	{
		line_is_blank = 1;

		for (i = 0; i < 128; i++)
		{
			if (chart[i] >= p)
				line_is_blank = 0;
		}
			
		if (line_is_blank)
			continue;

		for (i = 0; i < 128; i++)
		{
			if (chart[i] > 30)
				clr = ANSI_COLOR_RED;
			else if (chart[i] > 15)
				clr = ANSI_COLOR_YELLOW;
			else
				clr = ANSI_COLOR_BLUE;

			if (chart[i] >= 32)
				chr = '*';
			else if (chart[i] >= p)
				chr = '#';
			else if (p == 1 && chart[i] == -1)
				chr = '.';
			else
				chr = ' ';
			
			printf("%s%c", clr, chr);
		}
		printf(ANSI_COLOR_RESET "\n");
	}
}

static void summary(uint32_t *vals)
{
	uint32_t step, sw;

	sw = vals[0];
	step = vals[1];
	
	printf("SW %02x, from %d to %d step %u (%d)\n", sw, -512*(1<<step), 512*(1<<step)-1, 1<<step, step);
}

static void report(uint32_t *vals)
{
	unsigned int i, j;
	uint64_t acc, percent_weight;
	int chart[128];

	acc = 0;
	for (i = 0; i<1024; i++)
		acc += vals[i];

	percent_weight = acc / 256;
	
	if (!percent_weight)
	{
		printf("No data collected\n");
		return;
	}

	for (i = 0; i < 128; i++)
	{
		acc = 0;
		for (j = 0; j < 8; j++)
			acc += vals[i * 8 + j];

		chart[i] = (acc + percent_weight / 2) / percent_weight;
		if (chart[i] == 0 && acc != 0)
			chart[i] = -1;
	}

	print_chart(chart);
	printf("===60========50========40========30========20========10=========");
	printf("0========10========20========30========40========50========60===\n");
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
	printf("ERROR: argc is %d\n", argc);
	printf("Usage: %s <filename>\n", argv[0]);
}

int main(int argc, char* argv[])
{
	FILE *f;
	char buff[256];
	uint32_t vals[1024 + 1]; /* Data and step */
	int tail, cnt, i;

	if (argc != 2)
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

	tail = cnt = 0;
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
				summary(vals + 1024);
				report(vals);
				tail = cnt = 0;
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
