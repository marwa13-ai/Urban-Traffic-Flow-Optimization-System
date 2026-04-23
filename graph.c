#include "graph.h"

/* ══════════════════════════════════════════════════════════════
   GRAPH CREATION / DESTRUCTION
   ══════════════════════════════════════════════════════════════ */

Graph *create_graph(int num_nodes) {
    if (num_nodes <= 0 || num_nodes > MAX_NODES) return NULL;

    Graph *g = (Graph *)malloc(sizeof(Graph));
    if (!g) return NULL;

    g->num_nodes = num_nodes;
    g->num_edges = 0;

    g->adj = (Edge **)calloc(num_nodes, sizeof(Edge *));
    g->nodes = (Intersection *)calloc(num_nodes, sizeof(Intersection));

    if (!g->adj || !g->nodes) {
        free(g->adj);
        free(g->nodes);
        free(g);
        return NULL;
    }

    for (int i = 0; i < num_nodes; i++) {
        g->nodes[i].id  = i;
        g->nodes[i].row = 0;
        g->nodes[i].col = 0;
        g->nodes[i].congestion = 0.0;
        snprintf(g->nodes[i].name, 32, "N%d", i);
    }
    return g;
}

void free_graph(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_nodes; i++) {
        Edge *e = g->adj[i];
        while (e) {
            Edge *tmp = e->next;
            free(e);
            e = tmp;
        }
    }
    free(g->adj);
    free(g->nodes);
    free(g);
}

/* ══════════════════════════════════════════════════════════════
   EDGE OPERATIONS
   ══════════════════════════════════════════════════════════════ */

void add_road(Graph *g, int from, int to, double weight, int bidirectional) {
    if (!g || from < 0 || to < 0 || from >= g->num_nodes || to >= g->num_nodes)
        return;

    /* forward edge */
    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) return;
    e->target      = to;
    e->weight      = weight;
    e->base_weight = weight;
    e->status      = STATUS_OPEN;
    e->next        = g->adj[from];
    g->adj[from]   = e;
    g->num_edges++;

    if (bidirectional) {
        Edge *r = (Edge *)malloc(sizeof(Edge));
        if (!r) return;
        r->target      = from;
        r->weight      = weight;
        r->base_weight = weight;
        r->status      = STATUS_OPEN;
        r->next        = g->adj[to];
        g->adj[to]     = r;
        g->num_edges++;
    }
}

Edge *get_edge(Graph *g, int from, int to) {
    if (!g || from < 0 || from >= g->num_nodes) return NULL;
    Edge *e = g->adj[from];
    while (e) {
        if (e->target == to) return e;
        e = e->next;
    }
    return NULL;
}

void update_traffic(Graph *g, int from, int to, double new_weight) {
    if (!g) return;
    Edge *e = get_edge(g, from, to);
    if (e && e->status != STATUS_CLOSED) {
        e->weight = new_weight;
        /* Update node congestion heuristic */
        double ratio = new_weight / (e->base_weight > 0 ? e->base_weight : 1.0);
        g->nodes[from].congestion = fmin(1.0, (ratio - 1.0) / 4.0);
    }
}

void apply_incident(Graph *g, int from, int to, int status) {
    if (!g) return;
    Edge *e = get_edge(g, from, to);
    if (e) {
        e->status = status;
        if (status == STATUS_CLOSED)
            e->weight = INF;                /* effectively impassable */
        else if (status == STATUS_INCIDENT)
            e->weight = e->base_weight * 3.5;
    }
}

void reset_traffic(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_nodes; i++) {
        Edge *e = g->adj[i];
        while (e) {
            e->weight = e->base_weight;
            e->status = STATUS_OPEN;
            e = e->next;
        }
        g->nodes[i].congestion = 0.0;
    }
}

/* ══════════════════════════════════════════════════════════════
   CITY GRID CONSTRUCTION
   ══════════════════════════════════════════════════════════════ */

/*
 * Build an R×C rectangular grid graph.
 * Each intersection connects to its 4-directional neighbours.
 * Base weights are randomised [1.0, 5.0] to simulate varying road quality.
 */
Graph *build_grid_graph(int rows, int cols) {
    int n = rows * cols;
    Graph *g = create_graph(n);
    if (!g) return NULL;

    srand((unsigned)time(NULL));

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int id = r * cols + c;
            g->nodes[id].row = r;
            g->nodes[id].col = c;
            snprintf(g->nodes[id].name, 32, "R%dC%d", r, c);

            /* connect right */
            if (c + 1 < cols) {
                double w = 1.0 + (rand() % 40) / 10.0;
                add_road(g, id, id + 1, w, 1);
            }
            /* connect down */
            if (r + 1 < rows) {
                double w = 1.0 + (rand() % 40) / 10.0;
                add_road(g, id, id + cols, w, 1);
            }
        }
    }
    return g;
}

void print_graph(Graph *g) {
    if (!g) return;
    printf("Graph: %d nodes, %d directed edges\n", g->num_nodes, g->num_edges);
    for (int i = 0; i < g->num_nodes && i < 20; i++) {
        printf("  [%s] -> ", g->nodes[i].name);
        Edge *e = g->adj[i];
        while (e) {
            if (e->weight < INF)
                printf("%s(%.2f) ", g->nodes[e->target].name, e->weight);
            else
                printf("%s(CLOSED) ", g->nodes[e->target].name);
            e = e->next;
        }
        printf("\n");
    }
    if (g->num_nodes > 20)
        printf("  ... (%d more nodes not shown)\n", g->num_nodes - 20);
}

/* ══════════════════════════════════════════════════════════════
   ADJACENCY MATRIX  (for representation comparison)
   ══════════════════════════════════════════════════════════════ */

AdjMatrix *create_adj_matrix(int size) {
    AdjMatrix *m = (AdjMatrix *)malloc(sizeof(AdjMatrix));
    if (!m) return NULL;
    m->size   = size;
    m->matrix = (double **)malloc(size * sizeof(double *));
    if (!m->matrix) { free(m); return NULL; }

    for (int i = 0; i < size; i++) {
        m->matrix[i] = (double *)malloc(size * sizeof(double));
        if (!m->matrix[i]) {
            for (int j = 0; j < i; j++) free(m->matrix[j]);
            free(m->matrix);
            free(m);
            return NULL;
        }
        for (int j = 0; j < size; j++)
            m->matrix[i][j] = (i == j) ? 0.0 : INF;
    }
    return m;
}

void free_adj_matrix(AdjMatrix *m) {
    if (!m) return;
    for (int i = 0; i < m->size; i++) free(m->matrix[i]);
    free(m->matrix);
    free(m);
}

void set_matrix_edge(AdjMatrix *m, int from, int to, double w, int bidi) {
    if (!m || from < 0 || to < 0 || from >= m->size || to >= m->size) return;
    m->matrix[from][to] = w;
    if (bidi) m->matrix[to][from] = w;
}
