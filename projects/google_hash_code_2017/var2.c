
#include "hash17.h"

#include <stdio.h>
#include <stdlib.h>

void init_submission(void)
{
	int c, v;
	
	submissions = calloc(C, sizeof(struct submission));
	for (c = 0; c < C; c++)
	{
		submissions[c].load = 0;
		submissions[c].Varr = calloc(V, sizeof(struct VinCache));
		for (v = 0; v < V; v++)
			submissions[c].Varr[v].skip = 1;
			
	}
}

void requests_add_service_data(void)
{
	int r;

	for (r = 0; r < R; r++)
		requests[r].sVsize = Vsizes[requests[r].Rv];
}

static void sort_requests_fat_last(void)
{
	int r, rr, is_changed = 1;

	for (rr = 0; is_changed ; rr++)
	{
		is_changed = 0;
		for (r = 0; r < R -1 - rr; r++ )
		{
			if (requests[r].sVsize > requests[r + 1].sVsize)
			{
				struct request tmp = requests[r];
				requests[r] = requests[r+1];
				requests[r+1] = tmp;

				is_changed = 1;
			}
		}
	}
}

static void sub_sort_requests_popular_first(int Rlimit)
{
	int r, rr, is_changed = 1;

	for (rr = 0; is_changed ; rr++)
	{
		is_changed = 0;
		for (r = 0; r < Rlimit -1 - rr; r++ )
		{
			if (requests[r].Rn < requests[r + 1].Rn)
			{
				struct request tmp = requests[r];
				requests[r] = requests[r+1];
				requests[r+1] = tmp;

				is_changed = 1;
			}
		}
	}
}

void requests_sort_smart(void)
{
//	sort_requests_fat_last();
//	sub_sort_requests_popular_first(R/2);
	sub_sort_requests_popular_first(R);
}

void initial_fill(void)
{
	int r, k;

	for (r = 0; r < R; r++)
	{
		int Rv = requests[r].Rv;
		int Re = requests[r].Re;
		int Rn = requests[r].Rn;
		int sVsize = requests[r].sVsize;
		int K = endpoints[Re].K ; /* number of connected cache servers */

		/* Check that we have no connection */
		for (k = 0; k < K; k++) 
		{
			int c = endpoints[Re].connections[k].c; /* The ID of the cache server */
			
			if (!submissions[c].Varr[Rv].skip)
				break;
		}

		if (k != K)
		{
			requests[r].sKcurr = k;
			continue;
		}

		/* Find the best cache for it */
		for (k = 0; k < K; k++) 
		{
			int c = endpoints[Re].connections[k].c; /* The ID of the cache server */
			if (submissions[c].load + sVsize > X)
				continue;
			
			requests[r].sKcurr = k;
			submissions[c].load += sVsize;
			submissions[c].Varr[Rv].skip = 0;
			break;
		}
	}
}
