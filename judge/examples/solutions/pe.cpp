#include <iostream>
int main() {
    long long a, b;
    std::cin >> a >> b;
    std::cout << a << " " << b << " " << a + b << std::endl;  // 一行;期望是三行 → 仅空白差异 → PE
    return 0;
}
