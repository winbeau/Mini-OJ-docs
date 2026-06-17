#include <cstdio>
int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    double sum = 0, x;
    for (int i = 0; i < n; i++) { scanf("%lf", &x); sum += x; }
    printf("%.7f\n", sum / n);   // 比标准答案多两位小数,需 --special 才 AC
    return 0;
}
