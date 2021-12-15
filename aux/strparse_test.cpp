#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

int main() {
    string s = "\x1f";
    string s2 = "\"\\x2\"";
    cout << s << endl;
    cout << s2 << endl;
    return 0;
}