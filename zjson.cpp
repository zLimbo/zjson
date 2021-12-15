#include "zjson.h"

#define FMT_HEADER_ONLY
#include <errno.h>
#include <fmt/printf.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>

#define LOG(format)                               \
    do {                                          \
        fmt::print("{}:{}:", __FILE__, __LINE__); \
        fmt::print(format);                       \
        fmt::print("\n");                         \
    } while (0)

namespace zjson {

void Json::clear() {
    switch (type_) {
        case TYPE_STRING:
            if (value_.str) {
                free(value_.str);
            }
            break;
        default: break;
    }
    memset(&value_, 0, sizeof(value_));
    type_ = TYPE_NULL;
}

Json::Json() : type_(TYPE_NULL) { memset(&value_, 0, sizeof(value_)); }

Json::~Json() { clear(); }

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
    clear();
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
        case '\"': ret = parse_string(sv); break;
        case '[':
        case '{':
        default: ret = parse_number(sv);
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
    // TODO 暂时使用系统库的解析方式匹配测试用例
    // 检查
    const char *p = sv.data();
    if (*p == '-') {
        ++p;
    }
    if (*p == '0') {
        ++p;
    } else if (isdigit(*p)) {
        ++p;
        while (isdigit(*p)) {
            ++p;
        }
    } else {
        return PARSE_INVALID_VALUE;
    }
    if (*p == '.') {
        ++p;
        if (!isdigit(*p)) {
            return PARSE_INVALID_VALUE;
        }
        ++p;
        while (isdigit(*p)) {
            ++p;
        }
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') {
            ++p;
        }
        if (!isdigit(*p)) {
            return PARSE_INVALID_VALUE;
        }
        ++p;
        while (isdigit(*p)) ++p;
    }

    double number = strtod(sv.data(), nullptr);
    if (std::isinf(number)) {
        return PARSE_NUMBER_TOO_BIG;
    }

    sv = p;
    type_ = TYPE_NUMBER;
    value_.number = number;
    return PARSE_OK;
}

inline static int ch2hex(char ch) {
    int hex = 0;
    if (ch >= '0' && ch <= '9') {
        hex = ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        hex = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        hex = ch - 'A' + 10;
    } else {
        return -1;
    }
    return hex;
}

int Json::parse_hex4(std::string_view sv) {
    if (sv.size() < 4) {
        return -1;
    }
    int code = 0;
    for (int i = 0; i < 4; ++i) {
        int hex = ch2hex(sv[i]);
        if (hex < 0) {
            return -1;
        }
        code = (code << 4) + hex;
    }
    return code;
}

std::tuple<Ret, int> Json::encode_utf8(std::string_view sv) {
    size_t pos = 0;
    int code = parse_hex4(sv.substr(pos));
    if (code < 0) {
        return {PARSE_INVALID_UNICODE_HEX, 0};
    }
    pos += 4;
    if (code >= 0xD800 && code < 0xDC00) {
        if (pos + 2 >= sv.size() || sv[pos++] != '\\' || sv[pos++] != 'u') {
            return {PARSE_INVALID_UNICODE_SURROGATE, 0};
        }
        int surrogate = parse_hex4(sv.substr(pos));
        if (surrogate < 0 || !(surrogate >= 0xDC00 && surrogate < 0xE000)) {
            return {PARSE_INVALID_UNICODE_SURROGATE, 0};
        }
        code = 0x10000 + (code - 0xD800) * 0x400 + (surrogate - 0xDC00);
        pos += 4;
    }
    if (code < 0x80) {
        stack_push(code);
    } else if (code < 0x800) {
        stack_push(0xC0 | (0x1F & (code >> 6)));
        stack_push(0X80 | (0x3F & code));
    } else if (code < 0x10000) {
        stack_push(0xE0 | (0x0F & (code >> 12)));
        stack_push(0X80 | (0x3F & (code >> 6)));
        stack_push(0X80 | (0x3F & code));
    } else if (code < 0x10FFFF) {
        stack_push(0xF0 | (0x07 & (code >> 18)));
        stack_push(0X80 | (0x3F & (code >> 12)));
        stack_push(0X80 | (0x3F & (code >> 6)));
        stack_push(0X80 | (0x3F & code));
    } else {
        return {PARSE_INVALID_UNICODE_HEX, 0};
    }
    return {PARSE_OK, pos};
}

Ret Json::parse_string(std::string_view &sv) {
    size_t start_pos = stack_.size();
    size_t pos = 1;
    while (pos < sv.size() && sv[pos] != '\"') {
        unsigned char ch = sv[pos++];
        if (ch == '\\') {
            if (pos == sv.size()) {
                break;
            }
            ch = sv[pos++];
            switch (ch) {
                case 'b': stack_push('\b'); break;
                case 'f': stack_push('\f'); break;
                case 'n': stack_push('\n'); break;
                case 'r': stack_push('\r'); break;
                case 't': stack_push('\t'); break;
                case '/': stack_push('/'); break;
                case '\"': stack_push('\"'); break;
                case '\\': stack_push('\\'); break;
                case 'u': {
                    auto [ret, add] = encode_utf8(sv.substr(pos));
                    if (ret != PARSE_OK) {
                        return ret;
                    }
                    pos += add;
                } break;
                default: return PARSE_INVALID_STRING_ESCAPE;
            }
        } else if (ch < 0x20) {
            return PARSE_INVALID_STRING_CHAR;
        } else {
            stack_push(ch);
        }
    }
    if (pos == sv.size()) {
        return PARSE_MISS_QUOTATION_MARK;
    }
    size_t len = stack_.size() - start_pos;
    value_.str = (char *)malloc(len + 1);
    memcpy(value_.str, stack_.data() + start_pos, len);
    value_.str[len] = '\0';
    type_ = TYPE_STRING;

    stack_.resize(start_pos);
    sv = sv.substr(pos + 1);
    return PARSE_OK;
}

double Json::get_number() const {
    if (type_ != TYPE_NUMBER) {
        throw std::runtime_error("json value isn't number!");
    }
    return value_.number;
}

const char *Json::get_string() const {
    if (type_ != TYPE_STRING) {
        throw std::runtime_error("json value isn't string!");
    }
    return value_.str;
}

}  // namespace zjson
