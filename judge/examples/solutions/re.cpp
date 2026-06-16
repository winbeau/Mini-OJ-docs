#include <iostream>
int main() {
    long long a, b;
    std::cin >> a >> b;
    int* p = nullptr;
    *p = 42;                   // 空指针解引用 -> SIGSEGV -> RE
    std::cout << a + b << std::endl;
    return 0;
}
