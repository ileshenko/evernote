#include <stdio.h>
#include <stdlib.h>

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

static void report_1(void)
{
	int i;

	for (i = 0; i < C; i++)
		printf("%d ", submissions[i].load);

	printf("\n");
}

static void init_submission(void)
{
	int i;
	int Vall = 0;
	
	for (i = 0; i < V; i++)
		Vall += Vsizes[i];

//	printf("Vall %d %#x\n", Vall, Vall);

	submissions = calloc(C ,sizeof(struct submission));
	for (i = 0; i < C; i++)
	{
		submissions[i].load = Vall;
		submissions[i].Varr = calloc(V, sizeof(struct VinCache));
	}
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

static void remove_unused(void)
{
	int i, j;

	clear_service();

	for (i = 0; i < R ; i++)
	{
		int Re = requests[i].Re;
		int Rv = requests[i].Rv;
		int K = endpoints[Re].K; /* number of connected cache servers */
		struct connection *connections = endpoints[Re].connections;
		
			
		for (j = 0; j < K; j++)
		{
			int c = connections[j].c;
			submissions[c].Varr[Rv].service = 1;
		}
	}
	
	for (i = 0; i < C ; i++)
	{
		for (j = 0; j < V; j++)
		{
			if (submissions[i].Varr[j].skip)
				continue;

			if (submissions[i].Varr[j].service)
				continue;
			submissions[i].Varr[j].skip = 1;
			submissions[i].load -= Vsizes[j];
		}
	}
}

static void create_V_sorted_fat_first(void)
{
	int i, j;
	
	V_sorted_fat_first =  calloc(V, sizeof(int));

	for (i = 0; i < V; i++)
		V_sorted_fat_first[i] = i;

	for (i = 0; i < V - 1; i++)
	{
		for (j = 0; j < V - i - 1; j++)
		{
			if (Vsizes[V_sorted_fat_first[j]] > Vsizes[V_sorted_fat_first[j+1]])
			{
				int tmp = V_sorted_fat_first[j];
				V_sorted_fat_first[j] = V_sorted_fat_first[j + 1];
				V_sorted_fat_first[j + 1] =  tmp;
			}
		}
	}
}

static void create_V_sorted_ignored_first(void)
{
	int i, j;
	
	V_sorted_ignored_first = calloc(V, sizeof(int));

	for (i = 0; i < V; i++)
		V_sorted_ignored_first[i] = i;

	for (i = 0; i < V - 1; i++)
	{
		for (j = 0; j < V - i - 1; j++)
		{
			if (Vscores[V_sorted_ignored_first[j]] > Vscores[V_sorted_ignored_first[j+1]])
			{
				int tmp = V_sorted_ignored_first[j];
				V_sorted_ignored_first[j] = V_sorted_ignored_first[j + 1];
				V_sorted_ignored_first[j + 1] = tmp;
			}
		}
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

static unsigned long calcullate_Video_score(int Vunder_test)
{
	unsigned long score = 0;
	int r, k;

	for (r = 0; r < R; r++)
	{
		int Rv = requests[r].Rv;
		int Re = requests[r].Re;
		int Rn = requests[r].Rn;
		int K = endpoints[Re].K ; /* number of connected cache servers */
		struct connection *connections = endpoints[Re].connections;
		unsigned int request_score = 0;

		if (Rv != Vunder_test)
			continue;

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

static void remove_non_working(void)
{
	int v, c;
	unsigned long v_score;

	for (v = 0; v < V; v++)
	{
		v_score = calcullate_Video_score(v);
		
		for (c = 0; c < C; c++)
		{
			if (submissions[c].Varr[v].skip)
				continue;
			submissions[c].Varr[v].skip = 1;
			if (v_score == calcullate_Video_score(v))
			{
				submissions[c].load -= Vsizes[v];
			}
			else
			{
				submissions[c].Varr[v].skip = 0;
			}
		}
	}
}


#if 0
static void give_scores(void)
{
	int i,j;

	clear_service();

	for (i = 0; i < R; i++)
	{
		
	}
}
#endif

static void create_Vscores(void)
{
	int v;

	Vscores = calloc(V, sizeof(*Vscores));

	for (v = 0; v < V; v++)
		Vscores[v] = calcullate_Video_score(v);
}

static void final_remove_ignored_first(void)
{
	int c, i;

	for (c = 0; c < C; c++)
	{
		for (i = 0; i < V; i++)
		{
			int v = V_sorted_ignored_first[i];

			if (submissions[c].load <= X)
				break;
			
			if (submissions[c].Varr[v].skip)
				continue;
			submissions[c].Varr[v].skip = 1;
			submissions[c].load -= Vsizes[v];
		}
	}
}

static void final_remove_fat_first(void)
{
	int c, i;

	for (c = 0; c < C; c++)
	{
		for (i = 0; i < V; i++)
		{
			int v = V_sorted_fat_first[i];

			if (submissions[c].load <= X)
				break;
			
			if (submissions[c].Varr[v].skip)
				continue;
			submissions[c].Varr[v].skip = 1;
			submissions[c].load -= Vsizes[v];
		}
	}
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

int main(void)
{
	read_input();
	init_submission();

	sort_connections(); /* First connection - fastests */
	curr_score = calcullate_score();
//	printf("a %lld\n", curr_score);
	remove_unused();
//	report_1();

	create_Vscores();

	create_V_sorted_ignored_first();
	final_remove_ignored_first();

//	create_V_sorted_fat_first();
//	final_remove_fat_first();



//	remove_non_working();
	


//	report_1();
	

	curr_score = calcullate_score();
	printf("b %lld\n", curr_score * 1000 / SumRn);

	submit_report();
	return 0;
}
