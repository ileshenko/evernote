#include "hash17.h"

#include <stdio.h>
#include <stdlib.h>

int *V_sorted_fat_first;
int *V_sorted_ignored_first;

void init_submission(void)
{
	int i;
	int Vall = 0;
	
	for (i = 0; i < V; i++)
		Vall += Vsizes[i];

//	printf("Vall %d %#x\n", Vall, Vall);

	submissions = calloc(C, sizeof(struct submission));
	for (i = 0; i < C; i++)
	{
		submissions[i].load = Vall;
		submissions[i].Varr = calloc(V, sizeof(struct VinCache));
	}
}

void remove_unused(void)
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

static void create_Vscores(void)
{
	int v;

	Vscores = calloc(V, sizeof(*Vscores));

	for (v = 0; v < V; v++)
		Vscores[v] = calcullate_Video_score(v);
}

void final_remove_ignored_first(void)
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

void final_remove_fat_first(void)
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

void create_V_sorted_ignored_first(void)
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

