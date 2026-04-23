#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <math.h>

/* ─────────────────────────────────────────────
   CONSTANTS
   ───────────────────────────────────────────── */
#define MAX_NODES        10000
#define INF              DBL_MAX
#define MAX_PATH_LEN     10000
#define GRID_ROWS        10
#define GRID_COLS        10

/* Incident / road-status flags */
#define STATUS_OPEN      0
#define STATUS_INCIDENT  1
#define STATUS_CLOSED    2

/* ─────────────────────────────────────────────
   EDGE  (Adjacency-List node)
   ───────────────────────────────────────────── */
typedef struct Edge {
    int    target;          /* destination intersection  */
    double weight;          /* current travel cost (time)*/
    double base_weight;     /* original / free-flow cost */
    int    status;          /* OPEN | INCIDENT | CLOSED  */
    struct Edge *next;
} Edge;

/* ─────────────────────────────────────────────
   INTERSECTION  (vertex metadata)
   ───────────────────────────────────────────── */
typedef struct {
    int    id;
    int    row, col;        /* grid position             */
    char   name[32];        /* human-readable label      */
    double congestion;      /* 0.0 – 1.0                 */
} Intersection;

/* ─────────────────────────────────────────────
   GRAPH  (Adjacency-List representation)
   ───────────────────────────────────────────── */
typedef struct {
    int            num_nodes;
    int            num_edges;
    Edge         **adj;         /* adj[v] → linked list of edges */
    Intersection  *nodes;       /* vertex metadata array         */
} Graph;

/* ─────────────────────────────────────────────
   ADJACENCY MATRIX  (for comparison)
   ───────────────────────────────────────────── */
typedef struct {
    int     size;
    double **matrix;
} AdjMatrix;

/* ─────────────────────────────────────────────
   ROUTE RESULT
   ───────────────────────────────────────────── */
typedef struct {
    int    path[MAX_PATH_LEN];
    int    length;          /* number of nodes in path   */
    double total_weight;    /* sum of edge weights       */
    int    hops;            /* number of edges           */
    double compute_ms;      /* wall-clock time in ms     */
} Route;

/* ─────────────────────────────────────────────
   TRAFFIC EVENT  (simulation log entry)
   ───────────────────────────────────────────── */
typedef struct {
    int    from, to;
    double old_weight, new_weight;
    int    event_type;      /* 0=congestion 1=incident 2=closure */
    int    timestep;
} TrafficEvent;

/* ─────────────────────────────────────────────
   FUNCTION PROTOTYPES – graph.c
   ───────────────────────────────────────────── */
Graph    *create_graph(int num_nodes);
void      free_graph(Graph *g);
void      add_road(Graph *g, int from, int to, double weight, int bidirectional);
Edge     *get_edge(Graph *g, int from, int to);
void      update_traffic(Graph *g, int from, int to, double new_weight);
void      apply_incident(Graph *g, int from, int to, int status);
void      reset_traffic(Graph *g);
Graph    *build_grid_graph(int rows, int cols);
void      print_graph(Graph *g);

AdjMatrix *create_adj_matrix(int size);
void       free_adj_matrix(AdjMatrix *m);
void       set_matrix_edge(AdjMatrix *m, int from, int to, double w, int bidi);

/* ─────────────────────────────────────────────
   FUNCTION PROTOTYPES – routing.c
   ───────────────────────────────────────────── */
Route bfs_shortest_path(Graph *g, int src, int dst);
Route dijkstra_shortest_path(Graph *g, int src, int dst);
Route congestion_aware_route(Graph *g, int src, int dst, double congestion_penalty);
void  print_route(Graph *g, Route *r, const char *label);

/* ─────────────────────────────────────────────
   FUNCTION PROTOTYPES – analytics.c
   ───────────────────────────────────────────── */
typedef struct {
    int    nodes;
    int    edges;
    size_t list_bytes;
    size_t matrix_bytes;
    double bfs_ms;
    double dijkstra_ms;
    double congestion_ms;
    double bfs_weight;
    double dijkstra_weight;
    double path_quality_ratio;  /* dijkstra_weight / bfs_weight */
} BenchResult;

BenchResult run_benchmark(int nodes, int rows, int cols);
void        run_scalability_study(void);
void        run_update_frequency_study(Graph *g, int src, int dst);
void        compare_representations(int max_nodes);
void        print_bench_header(void);
void        print_bench_row(BenchResult *r);
size_t      calc_list_memory(Graph *g);
size_t      calc_matrix_memory(int n);

#endif /* GRAPH_H */
