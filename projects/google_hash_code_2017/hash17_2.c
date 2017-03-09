
#include "hash17.h"
#include <stdio.h>
#include <stdlib.h>

struct submission *submissions;
unsigned long *Vscores;

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
	init_submission();

	sort_connections(); /* First connection - fastests */
	requests_add_service_data(); /* add Vsizes for quick calculation */

#if 0 /* V1 */

	curr_score = calcullate_score();
	printf("a %lld\n", curr_score * 1000 / SumRn);

	remove_unused();
	report_loads();

	create_Vscores();

	create_V_sorted_ignored_first();
	final_remove_ignored_first();

//	create_V_sorted_fat_first();
//	final_remove_fat_first();

//	remove_non_working();

#endif

#if 0 /* var2 */
	requests_add_service_data(); /* add Vsizes for quick calculation */
	requests_sort_smart();
//report_Requests();

	initial_fill(); /* Only one cache for request */
//	improve_fill();
#endif

	/* Var3 */
	videos_by_popularity(); /* creates Video_pop */
	requests_sort_video_pop_first();
	initial_fill(); /* Only one cache for request */

	report_loads();

	printf("b %lld\n", calcullate_score() * 1000 / SumRn);

	submit_report();
	return 0;
}
