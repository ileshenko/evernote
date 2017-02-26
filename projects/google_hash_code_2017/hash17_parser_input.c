
#include "hash17.h"

#include <stdio.h>
#include <stdlib.h>

int V, E, R, C, X;
int *Vsizes;
struct endpoint *endpoints;
struct request *requests;
int SumRn;

void read_input(void)
{
	int i, j;
	scanf("%d %d %d %d %d\n", &V, &E, &R, &C, &X);

	Vsizes = malloc(sizeof(int) * V);

	for (i = 0; i < V; i++)
		scanf("%d", Vsizes+i);

	endpoints = calloc(E,  sizeof(*endpoints));

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

	requests = calloc(R, sizeof(*requests));

	for (i = 0; i < R; i++)
	{
		scanf("%d %d %d\n", &requests[i].Rv, &requests[i].Re, &requests[i].Rn);
		SumRn += requests[i].Rn;
	}
}

