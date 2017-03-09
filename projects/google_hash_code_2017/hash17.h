
struct connection {
	int c; /* The ID of the cache server */
	int Lc; /* Tha latency from this server */
	int Sc; /* Score Ld-Lc */
};

struct request {
	int Rv; /* Video */
	int Re; /* Endpoint */
	int Rn; /* number */
	/* service values */
	int sVsize;
	int sKcurr;
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
extern int *V_sorted_fat_first;
extern int *V_sorted_ignored_first;



void read_input(void);
void init_submission(void);

/* V1 only */
void remove_unused(void);
void final_remove_ignored_first(void);
void final_remove_fat_first(void);
void create_V_sorted_ignored_first(void);

/* V2 only */
void requests_add_service_data(void);
void requests_sort_smart(void);
void initial_fill(void);

/* V3 only */
void videos_by_popularity(void); /* creates Video_pop */
void requests_sort_video_pop_first(void);
