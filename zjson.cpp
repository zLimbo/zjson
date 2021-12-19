#include "zjson.h"

#include <errno.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace zjson {

const char *LITERAL_NULL = "null";
const char *LITERAL_TRUE = "true";
const char *LITERAL_FALSE = "false";

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
        case TYPE_OBJECT:
            if (value_.object) {
                delete value_.object;
                value_.object = nullptr;
            }
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
        case TYPE_STRING:
            value_.str = new std::string(*other.value_.str);
            break;
        case TYPE_ARRAY:
            value_.array = new std::vector<Json>(*other.value_.array);
            break;
        case TYPE_OBJECT:
            value_.object = new std::vector<std::pair<std::string, Json>>(
                *other.value_.object);
            break;
        default: break;
    }
}

Json::Json(const Json &other) { copy(other); }

Json &Json::operator=(const Json &other) {
    if (this != &other) {
        clear();
        copy(other);
    }
    return *this;
}

void Json::move(Json &other) {
    type_ = other.type_;
    memcpy(&value_, &other.value_, sizeof(value_));
    memset(&other.value_, 0, sizeof(other.value_));
}

Json::Json(Json &&other) noexcept { move(other); }

Json &Json::operator=(Json &&other) noexcept {
    if (this != &other) {
        clear();
        move(other);
    }
    return *this;
}

Json::~Json() { clear(); }

Ret Json::parse(const char *text) {
    clear();

    Ret ret = parse_text(text);
    // assert(stack_.size() == 0);

    if (ret != PARSE_OK) return ret;

    parse_whitespace(text);
    if (*text) {
        clear();
        return PARSE_ROOT_NOT_SINGULAR;
    }

    return ret;
}

void Json::stack_push(char ch) { stack_.push_back(ch); }

void Json::stack_push(const char *s) {
    while (*s) stack_.push_back(*s++);
}

Ret Json::parse_text(const char *&text) {
    parse_whitespace(text);
    if (!*text) return PARSE_EXPECT_VALUE;
    switch (*text) {
        case 'n': return parse_literal(text, LITERAL_NULL, TYPE_NULL);
        case 't': return parse_literal(text, LITERAL_TRUE, TYPE_TRUE);
        case 'f': return parse_literal(text, LITERAL_FALSE, TYPE_FALSE);
        case '\"': return parse_string(text);
        case '[': return parse_array(text);
        case '{': return parse_object(text);
        default: return parse_number(text);
    }
    return PARSE_INVALID_VALUE;
}

inline void Json::check_type(Type type, const char *msg) const {
    if (type_ != type) {
        std::string error_msg = std::string("text value isn't' ") + msg + "!";
        throw std::runtime_error(error_msg);
    }
}

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
inline void Json::parse_whitespace(const char *&text) {
    while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') {
        ++text;
    }
}

Ret Json::parse_literal(const char *&text, std::string_view literal,
                        Type type) {
    for (char c : literal) {
        if (*text++ != c) return PARSE_INVALID_VALUE;
    }
    type_ = type;
    return PARSE_OK;
}

Ret Json::parse_number(const char *&text) {
    // TODO 暂时使用系统库的解析方式匹配测试用例
    // 检查
    const char *p = text;
    if (*p == '-') ++p;
    if (*p == '0') {
        ++p;
    } else if (isdigit(*p)) {
        ++p;
        while (isdigit(*p)) ++p;
    } else {
        return PARSE_INVALID_VALUE;
    }
    if (*p == '.') {
        ++p;
        if (!isdigit(*p)) return PARSE_INVALID_VALUE;
        ++p;
        while (isdigit(*p)) ++p;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (!isdigit(*p)) return PARSE_INVALID_VALUE;
        ++p;
        while (isdigit(*p)) ++p;
    }

    double number = strtod(text, nullptr);
    if (std::isinf(number)) return PARSE_NUMBER_TOO_BIG;

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
        if (hex < 0) return -1;
        code = (code << 4) + hex;
    }
    return code;
}

Ret Json::encode_utf8(const char *&text) {
    int code = parse_hex4(text);
    if (code < 0) return PARSE_INVALID_UNICODE_HEX;
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

Ret Json::parse_string_raw(const char *&text) {
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
                    if (ret != PARSE_OK) return ret;
                } break;
                default: return PARSE_INVALID_STRING_ESCAPE;
            }
        } else if (ch < 0x20) {
            return PARSE_INVALID_STRING_CHAR;
        } else {
            stack_push(ch);
        }
    }
    if (*text++ != '\"') return PARSE_MISS_QUOTATION_MARK;
    return PARSE_OK;
}

Ret Json::parse_string(const char *&text) {
    size_t old_top = stack_.size();
    Ret ret = parse_string_raw(text);
    if (ret != PARSE_OK) return ret;
    value_.str =
        new std::string(stack_.data() + old_top, stack_.size() - old_top);
    stack_.resize(old_top);
    type_ = TYPE_STRING;
    return PARSE_OK;
}

Ret Json::parse_array(const char *&text) {
    ++text;
    parse_whitespace(text);
    if (!*text) {
        return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    } else if (*text == ']') {
        ++text;
        value_.array = new std::vector<Json>();
        type_ = TYPE_ARRAY;
        return PARSE_OK;
    }

    std::vector<Json> array;
    for (;;) {
        Json value;
        Ret ret = value.parse_text(text);
        if (ret != PARSE_OK) return ret;
        array.emplace_back(std::move(value));

        parse_whitespace(text);
        if (*text == ']') {
            ++text;
            break;
        } else if (*text != ',') {
            return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
        }
        ++text;
    }
    value_.array = new std::vector<Json>(std::move(array));
    type_ = TYPE_ARRAY;
    return PARSE_OK;
}

Ret Json::parse_object(const char *&text) {
    ++text;
    parse_whitespace(text);
    if (!*text) {
        return PARSE_MISS_COMMA_OR_CURLY_BRACKET;
    } else if (*text == '}') {
        ++text;
        value_.object = new std::vector<std::pair<std::string, Json>>();
        type_ = TYPE_OBJECT;
        return PARSE_OK;
    }

    std::vector<std::pair<std::string, Json>> object;
    for (;;) {
        int old_top = stack_.size();
        if (*text != '\"' || parse_string_raw(text) != PARSE_OK) {
            return PARSE_MISS_KEY;
        }
        parse_whitespace(text);
        if (*text++ != ':') return PARSE_MISS_COLON;

        std::string key(stack_.data() + old_top, stack_.size() - old_top);
        stack_.resize(old_top);

        Json value;
        Ret ret = value.parse_text(text);
        if (ret != PARSE_OK) return ret;

        object.emplace_back(std::move(key), std::move(value));

        parse_whitespace(text);
        if (*text == '}') {
            ++text;
            break;
        } else if (*text != ',') {
            return PARSE_MISS_COMMA_OR_CURLY_BRACKET;
        }
        ++text;
        parse_whitespace(text);
    }

    value_.object =
        new std::vector<std::pair<std::string, Json>>(std::move(object));
    type_ = TYPE_OBJECT;
    return PARSE_OK;
}

bool Json::is_bool(bool b) const {
    switch (type_) {
        case TYPE_TRUE: return b;
        case TYPE_FALSE: return !b;
        default: break;
    }
    throw std::runtime_error("text value isn't number!");
    return false;
}

double Json::get_number() const {
    check_type(TYPE_NUMBER, "number");
    return value_.number;
}

const char *Json::get_string() const {
    check_type(TYPE_STRING, "string");
    return value_.str->data();
}

size_t Json::get_array_size() const {
    check_type(TYPE_ARRAY, "array");
    return value_.array->size();
}

Json &Json::get_array_element(size_t idx) const {
    check_type(TYPE_ARRAY, "array");
    return (*value_.array)[idx];
}

size_t Json::get_object_size() const {
    check_type(TYPE_OBJECT, "object");
    return value_.object->size();
}

size_t Json::get_object_key_length(size_t idx) const {
    check_type(TYPE_OBJECT, "object");
    return (*value_.object)[idx].first.size();
}

const char *Json::get_object_key(size_t idx) const {
    check_type(TYPE_OBJECT, "object");
    return (*value_.object)[idx].first.data();
}

Json &Json::get_object_value(size_t idx) const {
    check_type(TYPE_OBJECT, "object");
    return (*value_.object)[idx].second;
}

inline char *Json::str_dup(const char *str, size_t len) {
    char *text = (char *)malloc(len + 1);
    memcpy(text, str, len);
    text[len] = '\0';
    return text;
}

char *Json::stringify_number() {
    char buf[32];
    int len = sprintf(buf, "%.17g", value_.number);
    return str_dup(buf, len);
}

void Json::stringify_hex4(int code) {
    char buf[5];
    for (int i = 3; i >= 0; --i) {
        int x = code % 16;
        buf[i] = x < 10 ? x + '0' : x - 10 + 'A';
        code /= 16;
    }
    buf[4] = '\0';
    stack_push(buf);
}

int Json::stringify_utf8(const char *str) {
    char ch = *str++;
    int count = 1;
    if ((ch & 0xF0) == 0xF0) {
        count = 4;
    } else if ((ch & 0xE0) == 0xE0) {
        count = 3;
    } else if ((ch & 0xC0) == 0xC0) {
        count = 2;
    }
    int code = ((ch << count) & 0xFF) >> count;
    for (int i = 1; i < count; ++i) {
        code = (code << 6) + (*str++ & 0x3F);
    }
    if (code < 0x10000) {
        stack_push("\\u");
        stringify_hex4(code);
    } else {
        code &= 0xFFFF;
        int H = code / 0x400 + 0xD800;
        int L = code % 0x400 + 0xDC00;
        stack_push("\\u");
        stringify_hex4(H);
        stack_push("\\u");
        stringify_hex4(L);
    }
    return count;
}

char *Json::stringify_string_raw(const char *str, int len) {
    size_t old_top = stack_.size();
    stack_push("\"");
    int pos = 0;
    while (pos < len) {
        switch (str[pos]) {
            case '\b': stack_push("\\b"); break;
            case '\f': stack_push("\\f"); break;
            case '\n': stack_push("\\n"); break;
            case '\r': stack_push("\\r"); break;
            case '\t': stack_push("\\t"); break;
            case '/': stack_push("/"); break;
            case '\"': stack_push("\\\""); break;
            case '\\': stack_push("\\\\"); break;
            default: {
                if (str[pos] < 0x20) {
                    pos += stringify_utf8(str + pos);
                    --pos;
                } else {
                    stack_push(str[pos]);
                }
            } break;
        }
        ++pos;
    }
    stack_push("\"");
    char *text = str_dup(stack_.data() + old_top, stack_.size() - old_top);
    stack_.resize(old_top);
    return text;
}

char *Json::stringify_string() {
    return stringify_string_raw(value_.str->data(), value_.str->size());
}

char *Json::stringify_array() {
    size_t old_top = stack_.size();
    stack_push('[');
    auto &array = *value_.array;
    for (size_t i = 0; i < array.size(); ++i) {
        char *subtext = array[i].stringify();
        stack_push(subtext);
        free(subtext);
        if (i != array.size() - 1) stack_push(',');
    }
    stack_push(']');
    char *text = str_dup(stack_.data() + old_top, stack_.size() - old_top);
    stack_.resize(old_top);
    return text;
}

char *Json::stringify_object() {
    size_t old_top = stack_.size();
    stack_push('{');
    auto &object = *value_.object;
    for (size_t i = 0; i < object.size(); ++i) {
        char *ktext = stringify_string_raw(object[i].first.data(),
                                           object[i].first.size());
        char *vtext = object[i].second.stringify();
        stack_push(ktext);
        stack_push(':');
        stack_push(vtext);
        free(ktext);
        free(vtext);
        if (i != object.size() - 1) stack_push(',');
    }
    stack_push('}');
    char *text = str_dup(stack_.data() + old_top, stack_.size() - old_top);
    stack_.resize(old_top);
    return text;
}

char *Json::stringify() {
    switch (type_) {
        case TYPE_NULL: return str_dup(LITERAL_NULL, 4);
        case TYPE_TRUE: return str_dup(LITERAL_TRUE, 4);
        case TYPE_FALSE: return str_dup(LITERAL_FALSE, 5);
        case TYPE_NUMBER: return stringify_number();
        case TYPE_STRING: return stringify_string();
        case TYPE_ARRAY: return stringify_array();
        case TYPE_OBJECT: return stringify_object();
        default: break;
    }
    return nullptr;
}

}  // namespace zjson
