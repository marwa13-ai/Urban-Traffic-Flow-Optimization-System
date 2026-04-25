#include "graph.h"

/* ==============================================================
   INTERNAL HELPERS
   ============================================================== */

/* Reconstruct path from predecessor array */
static int build_path(int *prev, int src, int dst, int *path_out) {
    if (prev[dst] == -1 && dst != src) return 0; /* no path */

    /* trace back */
    int tmp[MAX_PATH_LEN], len = 0;
    int cur = dst;
    while (cur != -1) {
        if (len >= MAX_PATH_LEN) return 0;
        tmp[len++] = cur;
        cur = prev[cur];
    }
    /* reverse */
    for (int i = 0; i < len; i++)
        path_out[i] = tmp[len - 1 - i];
    return len;
}

/* Sum edge weights along a path */
static double path_weight(Graph *g, int *path, int len) {
    double total = 0.0;
    for (int i = 0; i + 1 < len; i++) {
        Edge *e = get_edge(g, path[i], path[i + 1]);
        if (e) total += e->weight;
    }
    return total;
}

/* ==============================================================
   BFS  –  SHORTEST DISTANCE  (min hops)
   O(V + E)
   ============================================================== */

Route bfs_shortest_path(Graph *g, int src, int dst) {
    Route r;
    memset(&r, 0, sizeof(r));
    r.total_weight = INF;
    r.length = 0;

    if (!g || src < 0 || dst < 0 || src >= g->num_nodes || dst >= g->num_nodes)
        return r;

    clock_t t0 = clock();

    int  *visited = (int *)calloc(g->num_nodes, sizeof(int));
    int  *prev    = (int *)malloc(g->num_nodes * sizeof(int));
    int  *queue   = (int *)malloc(g->num_nodes * sizeof(int));

    if (!visited || !prev || !queue) {
        free(visited); free(prev); free(queue);
        return r;
    }

    for (int i = 0; i < g->num_nodes; i++) prev[i] = -1;

    int head = 0, tail = 0;
    queue[tail++] = src;
    visited[src]  = 1;

    while (head < tail) {
        int cur = queue[head++];
        if (cur == dst) break;

        Edge *e = g->adj[cur];
        while (e) {
            /* Skip closed roads */
            if (!visited[e->target] && e->status != STATUS_CLOSED && e->weight < INF) {
                visited[e->target] = 1;
                prev[e->target]    = cur;
                queue[tail++]      = e->target;
            }
            e = e->next;
        }
    }

    int path[MAX_PATH_LEN];
    int len = build_path(prev, src, dst, path);
    if (len > 0) {
        for (int i = 0; i < len && i < MAX_PATH_LEN; i++)
            r.path[i] = path[i];
        r.length       = len;
        r.hops         = len - 1;
        r.total_weight = path_weight(g, path, len);
    }

    clock_t t1 = clock();
    r.compute_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    free(visited); free(prev); free(queue);
    return r;
}

/* ==============================================================
   DIJKSTRA  –  SHORTEST TIME  (min weight)
   O((V + E) log V)  –  implemented with a simple array-based
   priority selection for portability; upgrade to min-heap for
   large-scale benchmarks (see analytics.c comments).
   ============================================================== */

Route dijkstra_shortest_path(Graph *g, int src, int dst) {
    Route r;
    memset(&r, 0, sizeof(r));
    r.total_weight = INF;
    r.length = 0;

    if (!g || src < 0 || dst < 0 || src >= g->num_nodes || dst >= g->num_nodes)
        return r;

    clock_t t0 = clock();

    int     n      = g->num_nodes;
    double *dist   = (double *)malloc(n * sizeof(double));
    int    *prev   = (int    *)malloc(n * sizeof(int));
    int    *done   = (int    *)calloc(n, sizeof(int));

    if (!dist || !prev || !done) {
        free(dist); free(prev); free(done);
        return r;
    }

    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0.0;

    for (int iter = 0; iter < n; iter++) {
        /* Find unvisited node with smallest dist */
        int u = -1;
        double best = INF;
        for (int v = 0; v < n; v++) {
            if (!done[v] && dist[v] < best) {
                best = dist[v];
                u = v;
            }
        }
        if (u == -1 || u == dst) break;
        done[u] = 1;

        /* Relax neighbours */
        Edge *e = g->adj[u];
        while (e) {
            int   t = e->target;
            double w = e->weight;
            /* Skip closed / impassable roads */
            if (!done[t] && e->status != STATUS_CLOSED && w < INF) {
                double nd = dist[u] + w;
                if (nd < dist[t]) {
                    dist[t] = nd;
                    prev[t] = u;
                }
            }
            e = e->next;
        }
    }

    int path[MAX_PATH_LEN];
    int len = build_path(prev, src, dst, path);
    if (len > 0) {
        for (int i = 0; i < len && i < MAX_PATH_LEN; i++)
            r.path[i] = path[i];
        r.length       = len;
        r.hops         = len - 1;
        r.total_weight = dist[dst] < INF ? dist[dst] : INF;
    }

    clock_t t1 = clock();
    r.compute_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    free(dist); free(prev); free(done);
    return r;
}

/* ==============================================================
   CONGESTION-AWARE ROUTING
   Edge effective cost = weight * (1 + penalty * congestion_src)
   This biases routes away from heavily loaded intersections.
   ============================================================== */

Route congestion_aware_route(Graph *g, int src, int dst, double congestion_penalty) {
    Route r;
    memset(&r, 0, sizeof(r));
    r.total_weight = INF;
    r.length = 0;

    if (!g || src < 0 || dst < 0 || src >= g->num_nodes || dst >= g->num_nodes)
        return r;

    clock_t t0 = clock();

    int     n    = g->num_nodes;
    double *dist = (double *)malloc(n * sizeof(double));
    int    *prev = (int    *)malloc(n * sizeof(int));
    int    *done = (int    *)calloc(n, sizeof(int));

    if (!dist || !prev || !done) {
        free(dist); free(prev); free(done);
        return r;
    }

    for (int i = 0; i < n; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0.0;

    for (int iter = 0; iter < n; iter++) {
        int u = -1;
        double best = INF;
        for (int v = 0; v < n; v++) {
            if (!done[v] && dist[v] < best) { best = dist[v]; u = v; }
        }
        if (u == -1 || u == dst) break;
        done[u] = 1;

        Edge *e = g->adj[u];
        while (e) {
            int    t = e->target;
            if (!done[t] && e->status != STATUS_CLOSED && e->weight < INF) {
                /* Penalise congested origin nodes */
                double cong_factor = 1.0 + congestion_penalty * g->nodes[u].congestion;
                double eff_w  = e->weight * cong_factor;
                double nd     = dist[u] + eff_w;
                if (nd < dist[t]) {
                    dist[t] = nd;
                    prev[t] = u;
                }
            }
            e = e->next;
        }
    }

    int path[MAX_PATH_LEN];
    int len = build_path(prev, src, dst, path);
    if (len > 0) {
        for (int i = 0; i < len && i < MAX_PATH_LEN; i++)
            r.path[i] = path[i];
        r.length       = len;
        r.hops         = len - 1;
        r.total_weight = path_weight(g, path, len); /* actual, not penalised */
    }

    clock_t t1 = clock();
    r.compute_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    free(dist); free(prev); free(done);
    return r;
}

/* ==============================================================
   ROUTE PRINTING
   ============================================================== */

void print_route(Graph *g, Route *r, const char *label) {
    printf("\n  [%s]\n", label);
    if (r->length == 0) {
        printf("    No path found.\n");
        return;
    }
    printf("    Path   : ");
    for (int i = 0; i < r->length; i++) {
        if (i > 0) printf(" -> ");
        if (g)
            printf("%s", g->nodes[r->path[i]].name);
        else
            printf("%d", r->path[i]);
    }
    printf("\n");
    printf("    Hops   : %d\n",    r->hops);
    if (r->total_weight < INF)
        printf("    Cost   : %.4f\n", r->total_weight);
    else
        printf("    Cost   : UNREACHABLE\n");
    printf("    Time   : %.4f ms\n", r->compute_ms);
}
