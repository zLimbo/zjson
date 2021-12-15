#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std;

int main() {
    double x = strtod("nan", NULL);
    printf("%f\n", x);
    cout << x << endl;
    bool ok = std::numeric_limits<double>::infinity() == abs(x);
    cout << ok << endl;
    cout << std::isnormal(0.1) << endl;

    x = strtod("-1e309", nullptr);
    cout << x << endl;
    cout << HUGE_VAL << endl;

    return 0;
}