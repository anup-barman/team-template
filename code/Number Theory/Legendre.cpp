// returns power of prime p in n!
ll getcnt(ll n, ll p) {
  ll ans = 0;
  while (n > 0) {
    n /= p;
    ans += n;
  }
  return ans;
}
