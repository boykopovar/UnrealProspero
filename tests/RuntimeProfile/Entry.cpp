#include <cstddef>

std::size_t UnrealProsperoRuntimeProbe(std::size_t count);

int main() {
    return UnrealProsperoRuntimeProbe(3) == 27 ? 0 : 1;
}
