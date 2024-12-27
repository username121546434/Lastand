#include "utils.h"

bool is_within(int a, int b, double c) {
    int d = std::abs(a - b);
    return d <= c;
}
