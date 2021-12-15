#define FMT_HEADER_ONLY
#include <fmt/printf.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "../zjson.h"

using namespace std;
using namespace fmt;

int main() {
    zjson::Json j1;
    zjson::Json j2(std::move(j1));
    vector<zjson::Json> v;
    v.emplace_back(std::move(j2));
    return 0;
}