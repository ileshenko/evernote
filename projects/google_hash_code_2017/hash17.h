
struct connection {
	int c; /* The ID of the cache server */
	int Lc; /* Tha latency from this server */
	int Sc; /* Score Ld-Lc */
};

struct request {
	int Rv; /* Video */
	int Re; /* Endpoint */
	int Rn; /* number */
};

struct endpoint {
	int Ld; /* Latency datacenter */
	int K; /* number of connected cache servers */
	struct connection *connections;
};

struct VinCache {
	int service;
	int skip;
};

struct submission {
	int load;
	struct VinCache *Varr;
};

extern int V, E, R, C, X;
extern int *Vsizes;
extern struct request *requests;
extern struct endpoint *endpoints;
extern int SumRn;

extern struct submission *submissions;
extern unsigned long *Vscores;
extern unsigned long long curr_score;
extern int *V_sorted_fat_first;
extern int *V_sorted_ignored_first;



void read_input(void);
