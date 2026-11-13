/**
 * DINIC'S MAX FLOW / MIN CUT CHEAT SHEET
 * -------------------------------------------------------------------------
 * TIME COMPLEXITY:
 *  - General capacities:       O(V^2 * E)
 *  - All capacities = 1:       O(E * min(V^(2/3), E^(1/2)))
 *  - Unit network (Bipartite): O(E * sqrt(V))
 *
 * HOW TO MODEL COMMON PROBLEMS:
 * 1. Bipartite Matching:
 *    - s -> Left (cap 1), Left -> Right (cap 1 or inf), Right -> t (cap 1).
 *    - Matched edges: check `flow_through[id] == 1` (or `e.flow == 1`).
 *    - Min Vertex Cover (Konig's): Left nodes with `d[u] == -1` U Right nodes with `d[v] != -1`.
 *    - Max Independent Set: Total nodes - Max Flow (complement of Min Vertex Cover).
 *
 * 2. Minimum Cut (Partitioning into S-side and T-side):
 *    - Value of Min Cut == Max Flow.
 *    - After `max_flow(s, t)`, node `u` is on the S-side iff `d[u] != -1`.
 *    - Cut edges: directed edges (u -> v) where `d[u] != -1` and `d[v] == -1`.
 *
 * 3. Project Selection / Max Weight Closure:
 *    - Gain `p[i] > 0`: add edge s -> i with cap `p[i]`, add `p[i]` to `total_positive`.
 *    - Cost `p[i] < 0`: add edge i -> t with cap `-p[i]`.
 *    - Dependency (if i chosen, MUST choose j): add edge i -> j with cap `inf`.
 *    - Answer = `total_positive - max_flow(s, t)`. Chosen nodes have `d[u] != -1`.
 *
 * 4. Vertex Capacities / Vertex-Disjoint Paths:
 *    - Split vertex `u` into `u_in = u` and `u_out = u + V`.
 *    - Add internal edge `u_in -> u_out` with cap = vertex capacity (1 for disjoint paths).
 *    - Redirect all incoming edges to `u_in` and outgoing edges from `u_out`.
 * -------------------------------------------------------------------------
 */
const int N = 5010;
const ll inf = 1LL << 61;

struct Dinic {
  struct edge {
    int to;   // Endpoint vertex of this directed edge
    int rev;  // Index of the reverse edge in `g[to]`
    ll flow;  // Current flow sent along this edge (can be negative on reverse edges)
    ll w;     // Capacity (maximum allowed flow) of this edge
    int id;   // Optional index (0..m-1) to query flow on this edge after running max_flow
  };

  int n;                    // Total number of vertices allocated
  int s, t;                 // Source (s) and Sink (t) vertices
  int mxid;                 // Maximum edge `id` passed to `add_edge` (sizes `flow_through`)
  vector<int> d;            // BFS level/distance from `s` in residual graph (`d[u] != -1` => S-side of min-cut)
  vector<ll> flow_through;  // Stores final flow for edges with `id >= 0` (NOTE: changed to `ll` to prevent overflow!)
  vector<int> done;         // Current-arc pointer for `u` in DFS (avoids re-scanning saturated edges)
  vector<vector<edge>> g;   // Adjacency list of the residual network

  Dinic() {}
  Dinic(int _n) {
    n = _n + 10;  // Extra padding so 0-indexed and 1-indexed nodes up to `_n` are safe
    mxid = -1;
    g.resize(n);
  }

  // Adds a directed edge u -> v with capacity `w`.
  // Pass `id` (0-indexed or 1-indexed) if you need to inspect `flow_through[id]` later.
  void add_edge(int u, int v, ll w, int id = -1) {
    edge a = {v, (int)g[v].size(), 0, w, id};
    edge b = {u, (int)g[u].size(), 0, 0, -2};  // For undirected edge of cap `w`, change `0` to `w`
    g[u].emplace_back(a);
    g[v].emplace_back(b);
    mxid = max(mxid, id);
  }

  // Builds the level graph by finding shortest paths from `s` using unsaturated edges
  bool bfs() {
    d.assign(n, -1);
    d[s] = 0;
    queue<int> q;
    q.push(s);
    while (!q.empty()) {
      int u = q.front();
      q.pop();
      for (auto& e : g[u]) {
        int v = e.to;
        if (d[v] == -1 && e.flow < e.w) d[v] = d[u] + 1, q.push(v);
      }
    }
    return d[t] != -1;  // Returns false when no augmenting path to `t` exists
  }

  // Pushes blocking flow along shortest paths in the level graph (`d[v] == d[u] + 1`)
  ll dfs(int u, ll flow) {
    if (u == t) return flow;
    for (int& i = done[u]; i < (int)g[u].size(); i++) {  // `int &i` updates `done[u]` in-place
      edge& e = g[u][i];
      if (e.w <= e.flow) continue;
      int v = e.to;
      if (d[v] == d[u] + 1) {
        ll nw = dfs(v, min(flow, e.w - e.flow));
        if (nw > 0) {
          e.flow += nw;
          g[v][e.rev].flow -= nw;
          return nw;
        }
      }
    }
    return 0;
  }

  // Computes max flow from `_s` to `_t`. Can be called again after adding more edges!
  ll max_flow(int _s, int _t) {
    s = _s;
    t = _t;
    ll flow = 0;
    while (bfs()) {
      done.assign(n, 0);
      while (ll nw = dfs(s, inf)) flow += nw;
    }
    flow_through.assign(mxid + 10, 0);
    for (int i = 0; i < n; i++)
      for (auto& e : g[i])
        if (e.id >= 0) flow_through[e.id] = e.flow;
    return flow;
  }

  // Helper: returns true if vertex `u` is on the Source side of the minimum cut
  bool left_of_min_cut(int u) {
    return d[u] != -1;
  }
};

int main() {
  int n, m;
  cin >> n >> m;
  Dinic F(n);  // Allocates up to n + 10 nodes automatically
  for (int i = 1; i <= m; i++) {
    int u, v;
    ll w;
    cin >> u >> v >> w;
    F.add_edge(u, v, w, i);  // Pass `i` as edge id if you want `F.flow_through[i]`
  }
  cout << F.max_flow(1, n) << '\n';
}