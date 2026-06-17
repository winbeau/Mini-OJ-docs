#include <cstdio>
#include <vector>
int main() {
    long long a, b;
    if (scanf("%lld %lld", &a, &b) != 2) return 0;
    std::vector<char> hog;
    for (int i = 0; i < 400; i++) hog.resize(hog.size() + 1024 * 1024, (char)1);  // ~400MB > 256MB
    printf("%lld\n", a + b + (long long)hog[0]);
    return 0;
}
