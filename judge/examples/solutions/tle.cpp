#include <iostream>
int main() {
    long long a, b;
    std::cin >> a >> b;
    volatile long long x = 0;
    while (true) x++;          // 死循环 -> 超时
    std::cout << a + b << std::endl;
    return 0;
}
