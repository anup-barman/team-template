const int MX = 2e5 + 10;
const int LOG = 20;
vector<int> g[MX];
int up[MX][LOG], depth[MX];
void dfs(int u, int p) {
  up[u][0] = p;
  for (int k = 1; k < LOG; ++k) {
    up[u][k] = up[up[u][k - 1]][k - 1];
  }
  for (const auto& v : g[u]) {
    if (v == p) continue;
    depth[v] = depth[u] + 1;
    dfs(v, u);
  }
}
int lca(int u, int v) {
  if (depth[u] < depth[v]) swap(u, v);
  int diff = depth[u] - depth[v];
  for (int i = LOG - 1; i >= 0; --i) {
    if ((diff >> i) & 1) {
      u = up[u][i];
    }
  }
  if (u == v) return u;
  for (int i = LOG - 1; i >= 0; --i) {
    if (up[u][i] != up[v][i]) {
      u = up[u][i];
      v = up[v][i];
    }
  }
  return up[u][0];
}