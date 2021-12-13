#include "zjson.h"

#include <cassert>

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

Ret Json::parse_literal(std::string_view &sv,
                                  std::string_view &literal_value, Type &type) {
    if (sv.substr(0, literal_value.size()) != literal_value) {
        return PARSE_FAIL;
    }
    sv = sv.substr(literal_value.size());
    type_ = type;
    return PARSE_OK;
}

Ret Json::parse_null(std::string_view &sv) {
    if (sv.substr(0, LITERAL_NULL.size()) != LITERAL_NULL) {
        return PARSE_FAIL;
    }
    sv = sv.substr(LITERAL_NULL.size());
    type_ = TYPE_NULL;
    return PARSE_OK;
}

Ret zjson::Json::parse_true(std::string_view &sv) {
    if (sv.substr(0, LITERAL_TRUE.size()) != LITERAL_TRUE) {
        return PARSE_FAIL;
    }
    sv = sv.substr(LITERAL_TRUE.size());
    type_ = TYPE_TRUE;
    return PARSE_OK;
}

Ret zjson::Json::parse_false(std::string_view &sv) {
    if (sv.substr(0, LITERAL_FALSE.size()) != LITERAL_FALSE) {
        return PARSE_FAIL;
    }
    sv = sv.substr(LITERAL_FALSE.size());
    type_ = TYPE_FALSE;
    return PARSE_OK;
}

Ret Json::parse(std::string_view sv) {
    parse_whitespace(sv);
    if (sv.empty()) {
        return PARSE_ROOT_IS_EMPTY;
    }
    Ret ret = PARSE_FAIL;
    switch (sv[0]) {
        case 'n':
            ret = parse_null(sv);
            break;
        case 't':
            ret = parse_true(sv);
            break;
        case 'f':
            ret = parse_false(sv);
            break;
        case '[':
        case '{':
        case '\"':
        default:  // number
            break;
    }

    if (ret == PARSE_FAIL) {
        return PARSE_FAIL;
    }

    parse_whitespace(sv);
    if (!sv.empty()) {
        return PARSE_ROOT_NOT_SINGULAR;
    }

    return ret;
}

}  // namespace zjson
