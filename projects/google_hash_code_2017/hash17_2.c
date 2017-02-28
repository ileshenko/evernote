
#include "hash17.h"
#include <stdio.h>
#include <stdlib.h>

struct submission *submissions;
unsigned long *Vscores;

#if 0
struct connection {
	int c; /* The ID of the cache server */
	int Lc; /* Tha latency from this server */
	int Sc; /* Score Ld-Lc */
};

static struct request {
	int Rv; /* Video */
	int Re; /* Endpoint */
	int Rn; /* number */
} *requests;


static int V, E, R, C, X;
static int *Vsizes;
static unsigned long *Vscores;
static int SumRn;
static unsigned long long curr_score;

static struct endpoint {
	int Ld; /* Latency datacenter */
	int K; /* number of connected cache servers */
	struct connection *connections;
} *endpoints;


struct VinCache {
	int service;
	int skip;
};

static struct submission {
	int load;
	struct VinCache *Varr;
} *submissions;

static int *V_sorted_fat_first;
static int *V_sorted_ignored_first;

static void read_input(void)
{
	int i, j;
	scanf("%d %d %d %d %d\n", &V, &E, &R, &C, &X);

	Vsizes = malloc(sizeof(int) * V);

	for (i = 0; i < V; i++)
		scanf("%d", Vsizes+i);

	endpoints = malloc(sizeof(struct endpoint) * E);

	for (i = 0; i < E; i++)
	{
		scanf("%d %d\n", &endpoints[i].Ld, &endpoints[i].K);
		endpoints[i].connections = malloc(sizeof(struct connection) * endpoints[i].K);
		for (j = 0; j < endpoints[i].K; j++)
		{
			scanf("%d %d", &endpoints[i].connections[j].c, &endpoints[i].connections[j].Lc);
			endpoints[i].connections[j].Sc = endpoints[i].Ld - endpoints[i].connections[j].Lc;
		}
	}

	requests = malloc(sizeof(struct request) * R);

	for (i = 0; i < R; i++)
	{
		scanf("%d %d %d\n", &requests[i].Rv, &requests[i].Re, &requests[i].Rn);
		SumRn += requests[i].Rn;
	}
}
#endif

static void sort_connections(void)
{
	int i, j, l;

	for (i = 0; i < E; i++)
	{
		int K = endpoints[i].K;
		struct connection *connections = endpoints[i].connections;

		for (j = 0; j < K - 1; j ++)
		{
			for (l = 0; l < K - j - 1; l++)
			{
				if (connections[j].Sc < connections[j+1].Sc)
				{
					struct connection tmp = connections[j];
					connections[j] = connections[j+1];
					connections[j+1] = tmp;
				}
			}
		}
	}
}

static void report_loads(void)
{
	int i;

	for (i = 0; i < C; i++)
		printf("%d ", submissions[i].load);

	printf("\n");
}

static void clear_service(void)
{
	int i, j;

	for (i = 0; i < C ; i++)
	{
		for (j = 0; j < V; j++)
			submissions[i].Varr[j].service = 0;
	}
}

static unsigned long long calcullate_score(void)
{
	unsigned long long score = 0;
	int r, k;

	for (r = 0; r < R; r++)
	{
		int Rv = requests[r].Rv;
		int Re = requests[r].Re;
		int Rn = requests[r].Rn;
		int K = endpoints[Re].K ; /* number of connected cache servers */
		struct connection *connections = endpoints[Re].connections;
		unsigned int request_score = 0;

		for (k = 0; k < K; k++)
		{
			int c = connections[k].c; /* The ID of the cache server */
			int Sc = connections[k].Sc; /* Score Ld-Lc */

			if (submissions[c].Varr[Rv].skip)
				continue;

			request_score = Sc;
			break;
		}
		score += request_score * Rn;
	}
	return score;
}

static void submit_report(void)
{
	int c, v;

	printf("%d\n", C);
	for (c = 0; c < C; c++)
	{
		printf("%d", c);
		for (v = 0; v < V; v++)
		{
			if (submissions[c].Varr[v].skip)
				continue;

			printf(" %d", v);
		}
		printf("\n");
	}
}

static void report_Requests(void)
{
	int r;

	printf("Requests %d\n", R);
	for (r = 0; r < 10; r++)
	{
		printf("Rv %d Re %d Rn %d sVsize %d sKcurr %d\n", requests[r].Rv,
			requests[r].Re, requests[r].Rn, requests[r].sVsize, requests[r].sKcurr);
	}
	printf("...\n");
	for (r = R - 10; r < R; r++)
	{
		printf("Rv %d Re %d Rn %d sVsize %d sKcurr %d\n", requests[r].Rv,
			requests[r].Re, requests[r].Rn, requests[r].sVsize, requests[r].sKcurr);
	}
	printf("==========\n");
		
}

int main(void)
{
	unsigned long long curr_score;

	read_input();
//report_Requests();
	init_submission();

	sort_connections(); /* First connection - fastests */
//	curr_score = calcullate_score();
//	printf("a %lld\n", curr_score * 1000 / SumRn);

#if 0 /* V1 */
	remove_unused();
//	report_loads();

	create_Vscores();

	create_V_sorted_ignored_first();
	final_remove_ignored_first();

//	create_V_sorted_fat_first();
//	final_remove_fat_first();

//	remove_non_working();

#endif

	requests_add_service_data(); /* add Vsizes for quick calculation */
	requests_sort_smart();
//report_Requests();

	initial_fill(); /* Only one cache for request */
//	improve_fill();

	report_loads();

	curr_score = calcullate_score();
	printf("b %lld\n", curr_score * 1000 / SumRn);

	submit_report();
	return 0;
}
