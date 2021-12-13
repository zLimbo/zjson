#include "zjson.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace zjson {

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
inline void Json::parse_whitespace(std::string_view &sv) {
    size_t pos = 0;
    while (pos < sv.size() && (sv[pos] == ' ' || sv[pos] == '\t' ||
                               sv[pos] == '\n' || sv[pos] == '\r')) {
        ++pos;
    }
    sv = sv.substr(pos);
}

Ret Json::parse_literal(std::string_view &sv, std::string_view literal_value,
                        Type type) {
    if (sv.substr(0, literal_value.size()) != literal_value) {
        return PARSE_INVALID_VALUE;
    }
    sv = sv.substr(literal_value.size());
    type_ = type;
    return PARSE_OK;
}

Ret Json::parse(std::string_view sv) {
    parse_whitespace(sv);
    if (sv.empty()) {
        return PARSE_EXPECT_VALUE;
    }
    Ret ret = PARSE_INVALID_VALUE;
    switch (sv[0]) {
        case 'n':
            // ret = parse_null(sv);
            ret = parse_literal(sv, LITERAL_NULL, TYPE_NULL);
            break;
        case 't':
            // ret = parse_true(sv);
            ret = parse_literal(sv, LITERAL_TRUE, TYPE_TRUE);
            break;
        case 'f':
            // ret = parse_false(sv);
            ret = parse_literal(sv, LITERAL_FALSE, TYPE_FALSE);
            break;
        case '[':
        case '{':
        case '\"':
        default:
            ret = parse_number(sv);
    }

    if (ret != PARSE_OK) {
        return ret;
    }

    parse_whitespace(sv);
    if (!sv.empty()) {
        clear();
        return PARSE_ROOT_NOT_SINGULAR;
    }

    return ret;
}

Ret Json::parse_number(std::string_view &sv) {
    Ret ret = PARSE_INVALID_VALUE;
    // TODO 暂时使用系统库的解析方式匹配测试用例
    if (!std::isdigit(sv[0]) && sv[0] != '-') {
        return PARSE_INVALID_VALUE;
    }
    char *pEnd;
    double number = strtod(sv.data(), &pEnd);
    if (sv.data() == pEnd || !std::isdigit(*(pEnd - 1))) {
        return PARSE_INVALID_VALUE;
    }
    if (std::isinf(number) || std::isnan(number)) {
        return PARSE_NUMBER_TOO_BIG;
    }
    sv = pEnd;
    type_ = TYPE_NUMBER;
    number_ = number;
    return PARSE_OK;
}

double Json::get_number() {
    if (type_ != TYPE_NUMBER) {
        throw std::runtime_error("json value isn't number!");
    }
    return number_;
}

void Json::clear() { type_ = TYPE_NULL; }

}  // namespace zjson
