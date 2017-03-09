#include "hash17.h"

#include <stdio.h>
#include <stdlib.h>

struct video {
	int v; /* general index */
	unsigned int pop; /* amount of requests */
	unsigned int service; 
};

struct video *Video_pops;

/* creates Video_pops */
void videos_by_popularity(void)
{
	int v, r;
	int is_changed = 1;

	Video_pops = calloc(V, sizeof(*Video_pops));

	for (v = 0; v < V; v++)
		Video_pops[v].v = v;

	for (r = 0; r < R; r++)
		Video_pops[requests[r].Rv].pop += requests[r].Rn;

	for (v = 0; v < V; v++)
		Video_pops[v].service = Video_pops[v].pop/Vsizes[v];

#if 0
	for (vv = 0; is_changed; vv++)
	{
		is_changed = 0;
		for (v = 0; v < V - 1 - vv; v ++)
		{
			if (Video_pops[v].pop < Video_pops[v+1].pop)
			{
				struct video tmp = Video_pops[v];
				Video_pops[v] = Video_pops[v+1];
				Video_pops[v+1] = tmp;

				is_changed = 1;
			}
		}
	}
#endif
}

void requests_sort_video_pop_first(void)
{
	int r, rr, is_changed = 1;

	for (rr = 0; is_changed ; rr++)
	{
		is_changed = 0;
		for (r = 0; r < R-1-rr; r++ )
		{
			if (Video_pops[requests[r].Rv].pop < Video_pops[requests[r+1].Rv].pop)
//			if (Video_pops[requests[r].Rv].service < Video_pops[requests[r+1].Rv].service)
			{
				struct request tmp = requests[r];
				requests[r] = requests[r+1];
				requests[r+1] = tmp;

				is_changed = 1;
			}
		}
	}
}

