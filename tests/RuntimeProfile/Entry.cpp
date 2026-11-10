#include <cstddef>

std::size_t AnyUnrealRuntimeProbe(std::size_t count);

int main() {
    return AnyUnrealRuntimeProbe(3) == 27 ? 0 : 1;
}
