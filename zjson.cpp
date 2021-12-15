#include "zjson.h"

#include <errno.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace zjson {

void Json::clear() {
    switch (type_) {
        case TYPE_STRING:
            if (value_.str) {
                delete value_.str;
                value_.str = nullptr;
            }
            break;
        case TYPE_ARRAY:
            if (value_.array) {
                delete value_.array;
                value_.array = nullptr;
            }
            break;
        default: break;
    }
    memset(&value_, 0, sizeof(value_));
    type_ = TYPE_NULL;
    stack_.clear();
}

Json::Json() : type_(TYPE_NULL) { memset(&value_, 0, sizeof(value_)); }

void Json::copy(const Json &other) {
    type_ = other.type_;
    switch (type_) {
        case TYPE_NUMBER: value_.number = other.value_.number; break;
        case TYPE_STRING: *value_.str = *other.value_.str; break;
        case TYPE_ARRAY: *value_.array = *other.value_.array; break;
        default: break;
    }
}

Json::Json(const Json &other) { copy(other); }

Json &Json::operator=(const Json &other) {
    if (this != &other) {
        copy(other);
    }
    return *this;
}

void Json::move(Json &other) {
    type_ = other.type_;
    memcpy(&value_, &other.value_, sizeof(value_));
    memset(&other.value_, 0, sizeof(other.value_));
}

Json::Json(Json &&other) { move(other); }

Json &Json::operator=(Json &&other) {
    if (this != &other) {
        clear();
        move(other);
    }
    return *this;
}

Json::~Json() { clear(); }

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
inline void Json::parse_whitespace(const char *&text) {
    while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') {
        ++text;
    }
}

Ret Json::parse_literal(const char *&text, std::string_view literal_value,
                        Type type) {
    for (char c : literal_value) {
        if (*text++ != c) return PARSE_INVALID_VALUE;
    }
    type_ = type;
    return PARSE_OK;
}

Ret Json::parse(const char *text) {
    clear();

    Ret ret = parse_text(text);
    if (ret != PARSE_OK) {
        return ret;
    }

    parse_whitespace(text);
    if (*text) {
        clear();
        return PARSE_ROOT_NOT_SINGULAR;
    }

    return ret;
}

Ret Json::parse_text(const char *&text) {
    parse_whitespace(text);
    if (!*text) {
        return PARSE_EXPECT_VALUE;
    }
    Ret ret = PARSE_INVALID_VALUE;
    switch (*text) {
        case 'n':
            // ret = parse_null(text);
            ret = parse_literal(text, LITERAL_NULL, TYPE_NULL);
            break;
        case 't':
            // ret = parse_true(text);
            ret = parse_literal(text, LITERAL_TRUE, TYPE_TRUE);
            break;
        case 'f':
            // ret = parse_false(text);
            ret = parse_literal(text, LITERAL_FALSE, TYPE_FALSE);
            break;
        case '\"': ret = parse_string(text); break;
        case '[': ret = parse_array(text); break;
        case '{':
        default: ret = parse_number(text);
    }
    return ret;
}

Ret Json::parse_number(const char *&text) {
    // TODO 暂时使用系统库的解析方式匹配测试用例
    // 检查
    const char *p = text;
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

    double number = strtod(text, nullptr);
    if (std::isinf(number)) {
        return PARSE_NUMBER_TOO_BIG;
    }

    text = p;
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

int Json::parse_hex4(const char *&text) {
    int code = 0;
    for (int i = 0; i < 4; ++i) {
        int hex = ch2hex(*text++);
        if (hex < 0) {
            return -1;
        }
        code = (code << 4) + hex;
    }
    return code;
}

Ret Json::encode_utf8(const char *&text) {
    int code = parse_hex4(text);
    if (code < 0) {
        return PARSE_INVALID_UNICODE_HEX;
    }
    if (code >= 0xD800 && code < 0xDC00) {
        if (*text++ != '\\' || *text++ != 'u') {
            return PARSE_INVALID_UNICODE_SURROGATE;
        }
        int surrogate = parse_hex4(text);
        if (surrogate < 0 || !(surrogate >= 0xDC00 && surrogate < 0xE000)) {
            return PARSE_INVALID_UNICODE_SURROGATE;
        }
        code = 0x10000 + (code - 0xD800) * 0x400 + (surrogate - 0xDC00);
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
        return PARSE_INVALID_UNICODE_HEX;
    }
    return PARSE_OK;
}

Ret Json::parse_string(const char *&text) {
    size_t start_pos = stack_.size();
    ++text;
    while (*text && *text != '\"') {
        unsigned char ch = *text++;
        if (ch == '\\') {
            switch (*text++) {
                case 'b': stack_push('\b'); break;
                case 'f': stack_push('\f'); break;
                case 'n': stack_push('\n'); break;
                case 'r': stack_push('\r'); break;
                case 't': stack_push('\t'); break;
                case '/': stack_push('/'); break;
                case '\"': stack_push('\"'); break;
                case '\\': stack_push('\\'); break;
                case 'u': {
                    Ret ret = encode_utf8(text);
                    if (ret != PARSE_OK) {
                        return ret;
                    }
                } break;
                default: return PARSE_INVALID_STRING_ESCAPE;
            }
        } else if (ch < 0x20) {
            return PARSE_INVALID_STRING_CHAR;
        } else {
            stack_push(ch);
        }
    }
    if (*text++ != '\"') {
        return PARSE_MISS_QUOTATION_MARK;
    }
    value_.str = new std::string(stack_.data() + start_pos);
    type_ = TYPE_STRING;
    stack_.resize(start_pos);
    return PARSE_OK;
}

Ret Json::parse_array(const char *&text) {
    parse_whitespace(text);

    // std::vector<Json> array;
    // for (;;) {
    //     Json value;
    //     text = text.substr(1);
    //     Ret ret = value.parse_text(text);
    //     if (ret != PARSE_OK) return ret;
    //     array.emplace_back(std::move(value));
    //     parse_whitespace(text);
    //     if (text.empty()) {
    //         return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    //     }
    //     if (text[0] == ']') {
    //         break;
    //     } else if (text[0] != ',') {
    //         return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    //     }
    // }
    // type_ = TYPE_ARRAY;
    // value_.array = new std::vector<Json>(std::move(array));
    // text = text.substr(1);
    return PARSE_OK;
}

bool Json::is_bool(bool b) const {
    switch (type_) {
        case TYPE_TRUE: return b;
        case TYPE_FALSE: return !b;
        default: break;
    }
    throw std::runtime_error("json value isn't number!");
    return false;
}

double Json::get_number() const {
    if (type_ != TYPE_NUMBER) {
        throw std::runtime_error("json value isn't number!");
    }
    return value_.number;
}

const std::string &Json::get_string() const {
    if (type_ != TYPE_STRING) {
        throw std::runtime_error("json value isn't string!");
    }
    return *value_.str;
}

const std::vector<Json> &Json::get_array() const {
    if (type_ != TYPE_ARRAY) {
        throw std::runtime_error("json value isn't array!");
    }
    return *value_.array;
}

const Json &Json::get_array_element(size_t idx) const {
    return get_array()[idx];
}

}  // namespace zjson
