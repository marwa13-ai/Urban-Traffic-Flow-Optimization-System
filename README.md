# 🚦 Urban Traffic Flow Optimization System

A comprehensive **urban traffic flow optimization and routing framework** written in C, modeling a city transportation network as a dynamic weighted graph. Intersections are represented as nodes and road segments as edges with time-varying travel costs derived from congestion, incidents, and road closures.

---

## 📌 Features

- **City network construction** — generates realistic grid-based city graphs
- **Dynamic edge-weight updates** — simulates rush hours, accidents, and road closures in real time
- **3 routing strategies** — BFS, Dijkstra, and Congestion-Aware routing
- **Incident & rerouting engine** — automatically finds alternate routes when roads are closed
- **Empirical performance analysis** — benchmarks runtime, memory, and path quality
- **Graph representation comparison** — Adjacency List vs. Adjacency Matrix
- **Scalability study** — tested from 4 nodes up to 900 nodes

---

## 🧠 Algorithms

### 1. BFS — Breadth-First Search
- **Goal**: Shortest path by number of hops (minimum intersections)
- **Complexity**: `O(V + E)`
- **Use case**: Fast routing ignoring road weights

### 2. Dijkstra — Shortest Time
- **Goal**: Minimum cost path (optimal travel time)
- **Complexity**: `O(V²)` with array, `O((V+E) log V)` with min-heap
- **Use case**: Finding the truly fastest route under current traffic

### 3. Congestion-Aware Routing
- **Goal**: Near-optimal path that avoids congested intersections
- **Complexity**: `O(V²)`
- **Formula**: `effective_cost = weight × (1 + penalty × node_congestion)`
- **Use case**: Smart routing that proactively avoids traffic jams

---

## 📁 Project Structure

```
ProjetAlgo/
│
├── main.c          # Simulation control & user interface (5-phase pipeline)
├── graph.h         # All data structures & function prototypes
├── graph.c         # Graph construction, edge management, traffic updates
├── routing.c       # BFS, Dijkstra, and Congestion-Aware routing engines
├── analytics.c     # Memory profiling, scalability & performance benchmarks
└── Makefile        # Build script (Linux/Mac)
```

---

## 🗂️ Data Structures

| Structure | Description |
|---|---|
| `Edge` | Linked-list node: target, weight, base_weight, status, next |
| `Intersection` | Vertex metadata: id, row, col, name, congestion level |
| `Graph` | Adjacency list: array of edge lists + intersection metadata |
| `AdjMatrix` | V×V double matrix for representation comparison |
| `Route` | Result: path array, hops, total weight, compute time |
| `TrafficEvent` | Simulation log: from/to, old/new weight, event type, timestep |

---

## ⚙️ Run

### On Windows (PowerShell)
```powershell
gcc main.c graph.c routing.c analytics.c -o traffic -lm
.\traffic.exe
```

## 📊 Simulation Phases

The program runs a **5-phase pipeline** automatically:

| Phase | Description |
|---|---|
| Phase 1 | City network construction (10×10 grid, 100 nodes, 360 edges) |
| Phase 2 | Static routing — compare BFS vs Dijkstra vs Congestion-Aware |
| Phase 3 | Dynamic simulation — 5 time-steps of random traffic incidents |
| Phase 4 | Incident & road-closure rerouting demonstration |
| Phase 5 | Full empirical analysis — scalability, memory, path quality |

---

## 📈 Empirical Results

### Path Quality (BFS vs Dijkstra)
> Dijkstra finds routes **13–43% cheaper** than BFS on average, since BFS minimizes hops, not cost.

### Memory: Adjacency List vs Matrix

| Nodes | List (KB) | Matrix (KB) | Ratio |
|---|---|---|---|
| 100 | 20.34 | 78.91 | ×3.88 |
| 500 | 103.46 | 1957.03 | ×18.92 |
| 1000 | 207.84 | 7820.31 | ×37.63 |

> For sparse city grids where `E ≈ 2V`, the adjacency list is drastically more memory-efficient.

### Scalability (Dijkstra runtime)

| Nodes | Dijkstra (ms) |
|---|---|
| 25 | ~0 |
| 225 | ~0 |
| 900 | ~3 |

---

## 🔁 Rerouting Demo

When the optimal path is fully closed, the system automatically finds a new route:

```
Before closure:  R0C0 → ... → R9C9   cost = 37.90
After closure:   R0C0 → ... → R9C9   cost = 42.50  (new path, all closures avoided)
```

---

## 📐 Complexity Summary

| Algorithm | Time | Space | Optimal? |
|---|---|---|---|
| BFS | O(V + E) | O(V) | ❌ hops only |
| Dijkstra (array) | O(V²) | O(V) | ✅ |
| Dijkstra (heap) | O((V+E) log V) | O(V) | ✅ |
| Congestion-Aware | O(V²) | O(V) | ~✅ |
| Adjacency List | — | O(V + E) | — |
| Adjacency Matrix | — | O(V²) | — |

---

## 🛠️ Requirements

- GCC compiler (`gcc --version`)
- Standard C99 (`-std=c99`)
- Math library (`-lm`)
- No external dependencies

---

## 
Project developed as part of an **Algorithms & Data Structures** course, implementing and empirically validating dynamic graph optimization techniques for urban mobility simulation.
