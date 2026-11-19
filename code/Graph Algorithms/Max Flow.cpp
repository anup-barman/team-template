/*
   V = total vertex Count
1. Matching:
   Max edges with no shared vertices.
2. Bipartite Minimum Vertex Cover:
   Size: MVC = Maximum Matching (Konig).
   Vertices: Left NOT reachable + Right reachable
   (alternating BFS from unmatched left vertices).
3. Maximum Independent Set:
   Max vertices with no edges between them.
   MIS = V - MVC.
4. Minimum Edge Cover:
   Min edges touching every vertex.
   MEC = V - Maximum Matching.
Note:
- Konig's theorem applies only to bipartite graphs.
- Edge Cover formula requires no isolated vertices.
*/
const long long inf = 1LL << 61;
struct Dinic {
  struct edge {
    int to, rev;
    long long flow, w;
    int id;
  };
  int n, s, t, mxid;
  vector<int> d, done;
  vector<long long> flow_through;
  vector<vector<edge>> g;

  Dinic() : n(0), s(-1), t(-1), mxid(-1) {}
  explicit Dinic(int _n) : n(_n + 10), s(-1), t(-1), mxid(-1), g(n) {}

  void add_edge(int u, int v, long long w, int id = -1) {
    int forward_rev = (int)g[v].size() + (u == v);
    int reverse_rev = (int)g[u].size();
    edge a = {v, forward_rev, 0, w, id};
    edge b = {u, reverse_rev, 0, 0, -2};
    g[u].emplace_back(a);
    g[v].emplace_back(b);
    mxid = max(mxid, id);
  }
  void add_undirected_edge(int u, int v, long long w, int id = -1) {
    add_edge(u, v, w, id);
    add_edge(v, u, w);
  }
  bool bfs() {
    d.assign(n, -1);
    d[s] = 0;
    queue<int> q;
    q.push(s);
    while (!q.empty()) {
      int u = q.front();
      q.pop();
      for (auto &e : g[u]) {
        int v = e.to;
        if (d[v] == -1 && e.flow < e.w)
          d[v] = d[u] + 1, q.push(v);
      }
    }
    return d[t] != -1;
  }
  long long dfs(int u, long long flow) {
    if (u == t) return flow;
    for (int &i = done[u]; i < (int)g[u].size(); i++) {
      edge &e = g[u][i];
      if (e.w <= e.flow) continue;
      int v = e.to;
      if (d[v] == d[u] + 1) {
        long long nw = dfs(v, min(flow, e.w - e.flow));
        if (nw > 0) {
          e.flow += nw;
          g[v][e.rev].flow -= nw;
          return nw;
        }
      }
    }
    return 0;
  }
  long long max_flow(int _s, int _t) {
    s = _s;
    t = _t;
    if (s == t) return 0;
    long long flow = 0;
    while (bfs()) {
      done.assign(n, 0);
      while (long long nw = dfs(s, inf)) flow += nw;
    }
    flow_through.assign(mxid + 10, 0);
    for (int i = 0; i < n; i++)
      for (auto e : g[i])
        if (e.id >= 0) flow_through[e.id] = e.flow;
    return flow;
  }
  // Vertices reachable from source in the final residual graph. Together
  // with unreachable vertices this gives a minimum cut after max_flow().
  vector<char> min_cut_reachable() const {
    vector<char> seen(n, false);
    if (s < 0) return seen;
    queue<int> q;
    seen[s] = true;
    q.push(s);
    while (!q.empty()) {
      int u = q.front();
      q.pop();
      for (const auto &e : g[u]) {
        if (!seen[e.to] && e.flow < e.w) {
          seen[e.to] = true;
          q.push(e.to);
        }
      }
    }
    return seen;
  }
};

