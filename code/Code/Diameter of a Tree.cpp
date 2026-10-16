// returns (diameter + 1) in dist and the node
array<int, 2> dfs(int u, int p = -1) {
  array<int, 2> ans = {u, 0};
  for (const auto& v : lst[u]) {
    if (v == p) continue;
    auto [node, dist] = dfs(v, u);
    if (dist > ans[1])
      ans = {node, dist};
  }
  ans[1]++;
  return ans;
}