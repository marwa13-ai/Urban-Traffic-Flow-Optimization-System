/*
 * ================================================================
 *   URBAN TRAFFIC FLOW OPTIMIZATION SYSTEM
 *   main.c -- Simulation Control & User Interface
 * ================================================================
 */

#include "graph.h"

static void print_banner(void);
static void demo_basic_routing(Graph *g, int src, int dst);
static void simulate_traffic(Graph *g, int steps, int src, int dst);
static void demo_rerouting(Graph *g, int src, int dst);
static void run_all_analytics(Graph *g, int src, int dst);

int main(void) {
    print_banner();

    printf("===============================================\n");
    printf("  PHASE 1 - CITY NETWORK CONSTRUCTION\n");
    printf("===============================================\n");

    int rows = GRID_ROWS, cols = GRID_COLS;
    Graph *g = build_grid_graph(rows, cols);
    if (!g) { fprintf(stderr, "ERROR: failed to build graph\n"); return 1; }

    printf("  Grid size : %d x %d\n", rows, cols);
    printf("  Nodes     : %d  (each = one intersection)\n", g->num_nodes);
    printf("  Edges     : %d  (each = one directed road)\n", g->num_edges);
    printf("  Memory    : %.2f KB (adjacency list)\n",
           (double)calc_list_memory(g) / 1024.0);
    printf("\n  Sample topology (first 20 nodes):\n");
    print_graph(g);

    int src = 0;
    int dst = g->num_nodes - 1;

    printf("\n===============================================\n");
    printf("  PHASE 2 - STATIC ROUTING  (%s -> %s)\n",
           g->nodes[src].name, g->nodes[dst].name);
    printf("===============================================\n");
    demo_basic_routing(g, src, dst);

    printf("\n===============================================\n");
    printf("  PHASE 3 - DYNAMIC TRAFFIC SIMULATION\n");
    printf("===============================================\n");
    simulate_traffic(g, 5, src, dst);

    printf("\n===============================================\n");
    printf("  PHASE 4 - INCIDENT & ROAD-CLOSURE REROUTING\n");
    printf("===============================================\n");
    demo_rerouting(g, src, dst);

    printf("\n===============================================\n");
    printf("  PHASE 5 - EMPIRICAL PERFORMANCE ANALYSIS\n");
    printf("===============================================\n");
    run_all_analytics(g, src, dst);

    free_graph(g);

    printf("\n================================================\n");
    printf("  Simulation complete. All memory freed.\n");
    printf("================================================\n");
    return 0;
}

static void print_banner(void) {
    printf("\n");
    printf("================================================================\n");
    printf("                                                                \n");
    printf("   URBAN TRAFFIC FLOW OPTIMIZATION SYSTEM                      \n");
    printf("   Dynamic Graph Routing & Analytics Framework                  \n");
    printf("                                                                \n");
    printf("   Algorithms : BFS | Dijkstra | Congestion-Aware              \n");
    printf("   Structures : Adjacency List | Adjacency Matrix               \n");
    printf("                                                                \n");
    printf("================================================================\n\n");
}

static void demo_basic_routing(Graph *g, int src, int dst) {
    printf("  Source      : node %d (%s)\n", src, g->nodes[src].name);
    printf("  Destination : node %d (%s)\n", dst, g->nodes[dst].name);

    Route rbfs  = bfs_shortest_path(g, src, dst);
    Route rdijk = dijkstra_shortest_path(g, src, dst);
    Route rcong = congestion_aware_route(g, src, dst, 2.0);

    print_route(g, &rbfs,  "BFS  - Shortest Distance (min hops)");
    print_route(g, &rdijk, "Dijkstra - Shortest Time  (min cost)");
    print_route(g, &rcong, "Congestion-Aware Routing  (penalty=2.0)");

    printf("\n  -- Algorithm Comparison --\n");
    printf("  BFS    cost : %.4f  (hops: %d)\n", rbfs.total_weight, rbfs.hops);
    printf("  Dijkstra    : %.4f  (hops: %d) - %.2f%% better\n",
           rdijk.total_weight, rdijk.hops,
           (rbfs.total_weight > 0 && rbfs.total_weight < INF)
               ? (rbfs.total_weight - rdijk.total_weight) / rbfs.total_weight * 100.0
               : 0.0);
    printf("  BFS   time  : %.4f ms\n", rbfs.compute_ms);
    printf("  Dijk  time  : %.4f ms\n", rdijk.compute_ms);
    printf("  Cong  time  : %.4f ms\n", rcong.compute_ms);
}

static void simulate_traffic(Graph *g, int steps, int src, int dst) {
    srand((unsigned)time(NULL));
    int n = g->num_nodes;

    printf("  Running %d time-steps of rush-hour simulation...\n\n", steps);

    Route prev_dijkstra;
    memset(&prev_dijkstra, 0, sizeof(prev_dijkstra));
    prev_dijkstra.length = 0;

    for (int step = 1; step <= steps; step++) {
        printf("  --- Time Step %d -----------------------------------\n", step);

        int num_updates = 5 + rand() % 6;
        int changed = 0;

        for (int u = 0; u < num_updates; u++) {
            int from = rand() % n;
            Edge *e  = g->adj[from];
            if (!e) continue;

            int hops = rand() % 5;
            for (int h = 0; h < hops && e->next; h++) e = e->next;

            double old_w = e->weight;
            double mult  = 1.5 + (rand() % 30) / 10.0;
            double new_w = e->base_weight * mult;

            if (fabs(old_w - new_w) > 0.01) {
                update_traffic(g, from, e->target, new_w);
                if (changed < 5) {
                    printf("    Incident: %s->%s  %.2f -> %.2f (x%.1f)\n",
                           g->nodes[from].name,
                           g->nodes[e->target].name,
                           old_w, new_w, mult);
                }
                changed++;
            }
        }
        if (changed > 5)
            printf("    ... (%d additional updates suppressed)\n", changed - 5);

        Route rdijk = dijkstra_shortest_path(g, src, dst);
        Route rcong = congestion_aware_route(g, src, dst, 2.0);

        int route_changed = (prev_dijkstra.length > 0 &&
                             rdijk.hops != prev_dijkstra.hops);
        printf("    Dijkstra cost  : %.4f (hops: %d)%s\n",
               rdijk.total_weight, rdijk.hops,
               route_changed ? "  <-- REROUTED!" : "");
        printf("    Congestion cost: %.4f (hops: %d)\n",
               rcong.total_weight, rcong.hops);

        prev_dijkstra = rdijk;
        printf("\n");
    }
    printf("  [Simulation complete]\n");
}

static void demo_rerouting(Graph *g, int src, int dst) {
    reset_traffic(g);

    printf("  Step A - Baseline Dijkstra route:\n");
    Route r_before = dijkstra_shortest_path(g, src, dst);
    print_route(g, &r_before, "Before incident");

    printf("\n  Step B - Applying ROAD CLOSURE on optimal-path edges...\n");
    if (r_before.length >= 2) {
        for (int i = 0; i + 1 < r_before.length; i++) {
            int u = r_before.path[i];
            int v = r_before.path[i + 1];
            apply_incident(g, u, v, STATUS_CLOSED);
            apply_incident(g, v, u, STATUS_CLOSED);
            printf("    CLOSED: %s <-> %s\n",
                   g->nodes[u].name, g->nodes[v].name);
        }
    } else {
        printf("    Blocking first row of edges as demonstration...\n");
        for (int i = 0; i < g->num_nodes / 10 && i < 5; i++) {
            Edge *e = g->adj[i];
            if (e) {
                apply_incident(g, i, e->target, STATUS_CLOSED);
                printf("    CLOSED: %s -> %s\n",
                       g->nodes[i].name, g->nodes[e->target].name);
            }
        }
    }

    printf("\n  Step C - Rerouting around closure:\n");
    Route r_after_dijk = dijkstra_shortest_path(g, src, dst);
    Route r_after_cong = congestion_aware_route(g, src, dst, 3.0);
    Route r_after_bfs  = bfs_shortest_path(g, src, dst);

    print_route(g, &r_after_dijk, "Dijkstra (post-closure)");
    print_route(g, &r_after_cong, "Congestion-Aware (post-closure, penalty=3.0)");
    print_route(g, &r_after_bfs,  "BFS (post-closure)");

    if (r_after_dijk.length == 0)
        printf("  [Dijkstra: no path exists - network partitioned!]\n");

    printf("\n  Step D - INCIDENT scenario (3.5x delay, not closed):\n");
    reset_traffic(g);
    int mid = g->num_nodes / 2;
    if (g->adj[mid]) {
        apply_incident(g, mid, g->adj[mid]->target, STATUS_INCIDENT);
        printf("  Incident applied on: %s -> %s (weight x3.5)\n",
               g->nodes[mid].name, g->nodes[g->adj[mid]->target].name);
    }
    Route ri_dijk = dijkstra_shortest_path(g, src, dst);
    Route ri_cong = congestion_aware_route(g, src, dst, 2.0);
    printf("  Dijkstra        : cost=%.4f, hops=%d\n", ri_dijk.total_weight, ri_dijk.hops);
    printf("  Congestion-Aware: cost=%.4f, hops=%d\n", ri_cong.total_weight, ri_cong.hops);

    reset_traffic(g);
}

static void run_all_analytics(Graph *g, int src, int dst) {
    run_scalability_study();

    reset_traffic(g);
    run_update_frequency_study(g, src, dst);

    compare_representations(1000);

    printf("\n================================================================\n");
    printf("   PATH QUALITY: BFS vs. DIJKSTRA (10 random pairs)\n");
    printf("================================================================\n");
    printf("\n%-8s %-8s %-12s %-12s %-12s %-10s\n",
           "Src", "Dst", "BFS_cost", "Dijk_cost", "Savings", "Savings%");
    printf("-------- -------- ------------ ------------ ------------ ----------\n");

    reset_traffic(g);
    srand(99);
    int n = g->num_nodes;
    double total_savings = 0;
    int valid = 0;

    for (int i = 0; i < 10; i++) {
        int s = rand() % n;
        int d = rand() % n;
        if (s == d) { d = (s + n / 2) % n; }

        Route rb = bfs_shortest_path(g, s, d);
        Route rd = dijkstra_shortest_path(g, s, d);

        double savings_pct = 0;
        if (rb.total_weight > 0 && rb.total_weight < INF && rd.total_weight < INF) {
            savings_pct = (rb.total_weight - rd.total_weight) / rb.total_weight * 100.0;
            total_savings += savings_pct;
            valid++;
        }

        printf("%-8d %-8d %-12.4f %-12.4f %-12.4f %-10.2f%%\n",
               s, d,
               (rb.total_weight < INF ? rb.total_weight : -1),
               (rd.total_weight < INF ? rd.total_weight : -1),
               (rb.total_weight < INF && rd.total_weight < INF
                   ? rb.total_weight - rd.total_weight : 0),
               savings_pct);
    }
    if (valid > 0)
        printf("  Average Dijkstra savings vs BFS: %.2f%%\n", total_savings / valid);

    printf("\n================================================================\n");
    printf("   THEORETICAL COMPLEXITY SUMMARY\n");
    printf("================================================================\n");
    printf("\n  Algorithm            Time Complexity     Space   Route Quality\n");
    printf("  -----------------    ----------------    ------  -------------------\n");
    printf("  BFS                  O(V + E)            O(V)    Min hops (not min cost)\n");
    printf("  Dijkstra (array)     O(V^2)              O(V)    Optimal (min cost)\n");
    printf("  Dijkstra (heap)      O((V+E) log V)      O(V)    Optimal (min cost)\n");
    printf("  Congestion-Aware     O(V^2)              O(V)    Near-optimal (avoids jams)\n");
    printf("\n  Graph Representation:\n");
    printf("  Adjacency List       O(V + E) space      O(E)    edge traversal\n");
    printf("  Adjacency Matrix     O(V^2)   space      O(1)    edge lookup\n");
    printf("\n  Edge weight update:\n");
    printf("  List:   O(E/V) avg to find edge, then O(1) update\n");
    printf("  Matrix: O(1) direct update - superior for frequent updates\n");
    printf("\n  Rerouting: incremental updates avoid full recomputation;\n");
    printf("  only affected portions of the priority queue are revised.\n");
}
