#include "graph.h"

/* ══════════════════════════════════════════════════════════════
   MEMORY CALCULATION
   ══════════════════════════════════════════════════════════════ */

size_t calc_list_memory(Graph *g) {
    if (!g) return 0;
    size_t total = sizeof(Graph)
                 + (size_t)g->num_nodes * sizeof(Edge *)
                 + (size_t)g->num_nodes * sizeof(Intersection);
    for (int i = 0; i < g->num_nodes; i++) {
        Edge *e = g->adj[i];
        while (e) { total += sizeof(Edge); e = e->next; }
    }
    return total;
}

size_t calc_matrix_memory(int n) {
    /* Outer pointer array + n rows of n doubles */
    return sizeof(double *) * (size_t)n
         + sizeof(double)   * (size_t)n * (size_t)n;
}

/* ══════════════════════════════════════════════════════════════
   SINGLE BENCHMARK RUN
   ══════════════════════════════════════════════════════════════ */

BenchResult run_benchmark(int nodes, int rows, int cols) {
    BenchResult res;
    memset(&res, 0, sizeof(res));

    Graph *g = build_grid_graph(rows, cols);
    if (!g) return res;

    res.nodes      = g->num_nodes;
    res.edges      = g->num_edges;
    res.list_bytes = calc_list_memory(g);
    res.matrix_bytes = calc_matrix_memory(g->num_nodes);

    int src = 0;
    int dst = g->num_nodes - 1;

    /* ── BFS ── */
    clock_t t0 = clock();
    Route rb = bfs_shortest_path(g, src, dst);
    clock_t t1 = clock();
    res.bfs_ms     = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;
    res.bfs_weight = rb.total_weight;

    /* ── Dijkstra ── */
    t0 = clock();
    Route rd = dijkstra_shortest_path(g, src, dst);
    t1 = clock();
    res.dijkstra_ms     = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;
    res.dijkstra_weight = rd.total_weight;

    /* ── Congestion-Aware ── */
    /* Simulate moderate congestion on 20% of edges */
    srand(42);
    for (int v = 0; v < g->num_nodes; v++) {
        Edge *e = g->adj[v];
        while (e) {
            if (rand() % 5 == 0)
                e->weight = e->base_weight * (1.5 + (rand() % 30) / 10.0);
            e = e->next;
        }
        g->nodes[v].congestion = (double)(rand() % 100) / 100.0;
    }
    t0 = clock();
    Route rc = congestion_aware_route(g, src, dst, 2.0);
    t1 = clock();
    res.congestion_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    /* Path-quality ratio: how much better is Dijkstra than BFS? */
    if (res.bfs_weight > 0 && res.bfs_weight < INF && res.dijkstra_weight < INF)
        res.path_quality_ratio = res.bfs_weight / res.dijkstra_weight;
    else
        res.path_quality_ratio = 1.0;

    (void)nodes; /* suppress warning – nodes == rows*cols */
    free_graph(g);
    return res;
}

/* ══════════════════════════════════════════════════════════════
   PRINT UTILITIES
   ══════════════════════════════════════════════════════════════ */

void print_bench_header(void) {
    printf("\n%-8s %-8s %-14s %-14s %-10s %-10s %-10s %-10s %-8s\n",
           "Nodes", "Edges",
           "List_KB", "Matrix_KB",
           "BFS_ms", "Dijk_ms", "Cong_ms",
           "BFS_cost", "Quality");
    printf("%s\n",
        "-------- -------- -------------- -------------- "
        "---------- ---------- ---------- ---------- --------");
}

void print_bench_row(BenchResult *r) {
    double list_kb   = (double)r->list_bytes   / 1024.0;
    double matrix_kb = (double)r->matrix_bytes / 1024.0;

    printf("%-8d %-8d %-14.2f %-14.2f %-10.4f %-10.4f %-10.4f %-10.4f %-8.3f\n",
           r->nodes, r->edges,
           list_kb, matrix_kb,
           r->bfs_ms, r->dijkstra_ms, r->congestion_ms,
           (r->bfs_weight < INF ? r->bfs_weight : -1.0),
           r->path_quality_ratio);
}

/* ══════════════════════════════════════════════════════════════
   SCALABILITY STUDY  (increasing grid sizes)
   Tests: 2×2 → 5×5 → 10×10 → 15×15 → 20×20 → 30×30
   ══════════════════════════════════════════════════════════════ */

void run_scalability_study(void) {
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║          SCALABILITY STUDY – INCREASING NETWORK SIZE           ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    int configs[][2] = {
        { 2,  2},
        { 5,  5},
        {10, 10},
        {15, 15},
        {20, 20},
        {30, 30}
    };
    int n_configs = (int)(sizeof(configs) / sizeof(configs[0]));

    print_bench_header();
    for (int i = 0; i < n_configs; i++) {
        int r = configs[i][0], c = configs[i][1];
        BenchResult res = run_benchmark(r * c, r, c);
        print_bench_row(&res);
    }

    printf("\n  Complexity Notes:\n");
    printf("  BFS     : O(V + E)       — E ≈ 2*(R*(C-1) + C*(R-1)) for grid\n");
    printf("  Dijkstra: O(V²) array    — optimal with min-heap: O((V+E)logV)\n");
    printf("  Memory  : List = O(V+E), Matrix = O(V²)\n");
}

/* ══════════════════════════════════════════════════════════════
   DYNAMIC UPDATE FREQUENCY STUDY
   Measures rerouting latency as update frequency increases.
   ══════════════════════════════════════════════════════════════ */

void run_update_frequency_study(Graph *g, int src, int dst) {
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║       DYNAMIC UPDATE FREQUENCY vs. REROUTING LATENCY          ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    printf("\n%-12s %-14s %-14s %-14s %-12s\n",
           "Updates/step", "Avg_BFS_ms", "Avg_Dijk_ms", "Avg_Cong_ms", "Route_Change%");
    printf("%-12s %-14s %-14s %-14s %-12s\n",
           "------------","----------","----------","----------","------------");

    int update_counts[] = {1, 3, 5, 10, 20};
    int n_tests = (int)(sizeof(update_counts) / sizeof(update_counts[0]));
    int n_steps = 20;
    int n       = g->num_nodes;

    srand(7);

    for (int ti = 0; ti < n_tests; ti++) {
        int updates_per_step = update_counts[ti];
        double total_bfs = 0, total_dijk = 0, total_cong = 0;
        int route_changes = 0;
        int prev_bfs_hops = -1;

        reset_traffic(g);

        for (int step = 0; step < n_steps; step++) {
            /* Apply random traffic updates */
            for (int u = 0; u < updates_per_step; u++) {
                int from = rand() % n;
                /* Pick a valid neighbour */
                Edge *e = g->adj[from];
                if (!e) continue;
                int hops = rand() % 5;
                for (int h = 0; h < hops && e->next; h++) e = e->next;
                double mult = 1.5 + (rand() % 50) / 10.0;
                update_traffic(g, from, e->target, e->base_weight * mult);
            }

            Route rb = bfs_shortest_path(g, src, dst);
            Route rd = dijkstra_shortest_path(g, src, dst);
            Route rc = congestion_aware_route(g, src, dst, 2.0);

            total_bfs  += rb.compute_ms;
            total_dijk += rd.compute_ms;
            total_cong += rc.compute_ms;

            if (prev_bfs_hops != -1 && rb.hops != prev_bfs_hops)
                route_changes++;
            prev_bfs_hops = rb.hops;
        }

        double pct = (double)route_changes / (n_steps - 1) * 100.0;
        printf("%-12d %-14.4f %-14.4f %-14.4f %-12.1f\n",
               updates_per_step,
               total_bfs  / n_steps,
               total_dijk / n_steps,
               total_cong / n_steps,
               pct);

        reset_traffic(g);
    }
}

/* ══════════════════════════════════════════════════════════════
   REPRESENTATION COMPARISON
   Lists vs. Matrix: memory and theoretical complexity.
   ══════════════════════════════════════════════════════════════ */

void compare_representations(int max_nodes) {
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║    REPRESENTATION COMPARISON: LIST vs. MATRIX                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    printf("\n%-8s %-14s %-14s %-10s %-10s\n",
           "Nodes", "List_KB", "Matrix_KB", "Ratio", "Matrix/List");
    printf("-------- -------------- -------------- ---------- ----------\n");

    int sizes[] = {10, 50, 100, 200, 500, 1000};
    int ns = (int)(sizeof(sizes) / sizeof(sizes[0]));

    for (int i = 0; i < ns; i++) {
        int n = sizes[i];
        if (n > max_nodes) break;

        /* For a sparse grid graph: E ≈ 2*(2n - 2*sqrt(n)) edges */
        int side  = (int)sqrt((double)n);
        int edges = 2 * (side * (side - 1)) * 2; /* bidirectional */
        size_t list_b   = sizeof(Graph)
                        + (size_t)n * sizeof(Edge *)
                        + (size_t)n * sizeof(Intersection)
                        + (size_t)edges * sizeof(Edge);
        size_t matrix_b = calc_matrix_memory(n);

        double list_kb   = list_b   / 1024.0;
        double matrix_kb = matrix_b / 1024.0;
        double ratio     = matrix_kb / (list_kb > 0 ? list_kb : 1.0);

        printf("%-8d %-14.2f %-14.2f %-10.2f  x%-9.2f\n",
               n, list_kb, matrix_kb, ratio, ratio);
    }

    printf("\n  Adjacency List  : O(V + E)  – ideal for sparse urban graphs\n");
    printf("  Adjacency Matrix: O(V²)    – fast O(1) edge lookup, costly memory\n");
    printf("  For city grids  : E ≈ 2V, so List ≪ Matrix for large V.\n");
}
