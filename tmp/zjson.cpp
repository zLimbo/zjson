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
        case Type::kString:
            if (value_.str) {
                delete value_.str;
                value_.str = nullptr;
            }
            break;
        case Type::kArray:
            if (value_.array) {
                delete value_.array;
                value_.array = nullptr;
            }
            break;
        case Type::kObject:
            if (value_.object) {
                delete value_.object;
                value_.object = nullptr;
            }
        default: break;
    }
    memset(&value_, 0, sizeof(value_));
    type_ = Type::kNull;
    stack_.clear();
}

Json::Json() : type_(Type::kNull) { memset(&value_, 0, sizeof(value_)); }

void Json::copy(const Json &other) {
    type_ = other.type_;
    switch (type_) {
        case Type::kNumber: value_.number = other.value_.number; break;
        case Type::kString: value_.str = new std::string(*other.value_.str); break;
        case Type::kArray:
            value_.array = new std::vector<Json>(*other.value_.array);
            break;
        case Type::kObject:
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

    if (ret != Ret::kParseOk) return ret;

    parse_whitespace(text);
    if (*text) {
        clear();
        return Ret::kParseRootNotSingular;
    }

    return ret;
}

void Json::stack_push(char ch) { stack_.push_back(ch); }

void Json::stack_push(const char *s) {
    while (*s) stack_.push_back(*s++);
}

Ret Json::parse_text(const char *&text) {
    parse_whitespace(text);
    if (!*text) return Ret::kParseExpectValue;
    switch (*text) {
        case 'n': return parse_literal(text, LITERAL_NULL, Type::kNull);
        case 't': return parse_literal(text, LITERAL_TRUE, Type::kTrue);
        case 'f': return parse_literal(text, LITERAL_FALSE, Type::kFalse);
        case '\"': return parse_string(text);
        case '[': return parse_array(text);
        case '{': return parse_object(text);
        default: return parse_number(text);
    }
    return Ret::kParseInvalidValue;
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
        if (*text++ != c) return Ret::kParseInvalidValue;
    }
    type_ = type;
    return Ret::kParseOk;
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
        return Ret::kParseInvalidValue;
    }
    if (*p == '.') {
        ++p;
        if (!isdigit(*p)) return Ret::kParseInvalidValue;
        ++p;
        while (isdigit(*p)) ++p;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (!isdigit(*p)) return Ret::kParseInvalidValue;
        ++p;
        while (isdigit(*p)) ++p;
    }

    double number = strtod(text, nullptr);
    if (std::isinf(number)) return Ret::kParseNumberTooBig;

    text = p;
    type_ = Type::kNumber;
    value_.number = number;
    return Ret::kParseOk;
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
    if (code < 0) return Ret::kParseInvalidUnicodeHex;
    if (code >= 0xD800 && code < 0xDC00) {
        if (*text++ != '\\' || *text++ != 'u') {
            return Ret::kParseInvalidUnicodeSurrogate;
        }
        int surrogate = parse_hex4(text);
        if (surrogate < 0 || !(surrogate >= 0xDC00 && surrogate < 0xE000)) {
            return Ret::kParseInvalidUnicodeSurrogate;
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
        return Ret::kParseInvalidUnicodeHex;
    }
    return Ret::kParseOk;
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
                    if (ret != Ret::kParseOk) return ret;
                } break;
                default: return Ret::kParseInvalidStringEscape;
            }
        } else if (ch < 0x20) {
            return Ret::kParseInvalidStringChar;
        } else {
            stack_push(ch);
        }
    }
    if (*text++ != '\"') return Ret::kParseMissQuotationMark;
    return Ret::kParseOk;
}

Ret Json::parse_string(const char *&text) {
    size_t old_top = stack_.size();
    Ret ret = parse_string_raw(text);
    if (ret != Ret::kParseOk) return ret;
    value_.str =
        new std::string(stack_.data() + old_top, stack_.size() - old_top);
    stack_.resize(old_top);
    type_ = Type::kString;
    return Ret::kParseOk;
}

Ret Json::parse_array(const char *&text) {
    ++text;
    parse_whitespace(text);
    if (!*text) {
        return Ret::kParseMissCommaOrSquareBracket;
    } else if (*text == ']') {
        ++text;
        value_.array = new std::vector<Json>();
        type_ = Type::kArray;
        return Ret::kParseOk;
    }

    std::vector<Json> array;
    for (;;) {
        Json value;
        Ret ret = value.parse_text(text);
        if (ret != Ret::kParseOk) return ret;
        array.emplace_back(std::move(value));

        parse_whitespace(text);
        if (*text == ']') {
            ++text;
            break;
        } else if (*text != ',') {
            return Ret::kParseMissCommaOrSquareBracket;
        }
        ++text;
    }
    value_.array = new std::vector<Json>(std::move(array));
    type_ = Type::kArray;
    return Ret::kParseOk;
}

Ret Json::parse_object(const char *&text) {
    ++text;
    parse_whitespace(text);
    if (!*text) {
        return Ret::kParseMissCommaOrCurlyBracket;
    } else if (*text == '}') {
        ++text;
        value_.object = new std::vector<std::pair<std::string, Json>>();
        type_ = Type::kObject;
        return Ret::kParseOk;
    }

    std::vector<std::pair<std::string, Json>> object;
    for (;;) {
        int old_top = stack_.size();
        if (*text != '\"' || parse_string_raw(text) != Ret::kParseOk) {
            return Ret::kParseMissKey;
        }
        parse_whitespace(text);
        if (*text++ != ':') return Ret::kParseMissColon;

        std::string key(stack_.data() + old_top, stack_.size() - old_top);
        stack_.resize(old_top);

        Json value;
        Ret ret = value.parse_text(text);
        if (ret != Ret::kParseOk) return ret;

        object.emplace_back(std::move(key), std::move(value));

        parse_whitespace(text);
        if (*text == '}') {
            ++text;
            break;
        } else if (*text != ',') {
            return Ret::kParseMissCommaOrCurlyBracket;
        }
        ++text;
        parse_whitespace(text);
    }

    value_.object =
        new std::vector<std::pair<std::string, Json>>(std::move(object));
    type_ = Type::kObject;
    return Ret::kParseOk;
}

bool Json::is_bool(bool b) const {
    switch (type_) {
        case Type::kTrue: return b;
        case Type::kFalse: return !b;
        default: break;
    }
    throw std::runtime_error("text value isn't number!");
    return false;
}

double Json::get_number() const {
    check_type(Type::kNumber, "number");
    return value_.number;
}

const char *Json::get_string() const {
    check_type(Type::kString, "string");
    return value_.str->data();
}

size_t Json::get_array_size() const {
    check_type(Type::kArray, "array");
    return value_.array->size();
}

Json &Json::get_array_element(size_t idx) const {
    check_type(Type::kArray, "array");
    return (*value_.array)[idx];
}

size_t Json::get_object_size() const {
    check_type(Type::kObject, "object");
    return value_.object->size();
}

size_t Json::get_object_key_length(size_t idx) const {
    check_type(Type::kObject, "object");
    return (*value_.object)[idx].first.size();
}

const char *Json::get_object_key(size_t idx) const {
    check_type(Type::kObject, "object");
    return (*value_.object)[idx].first.data();
}

Json &Json::get_object_value(size_t idx) const {
    check_type(Type::kObject, "object");
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
        case Type::kNull: return str_dup(LITERAL_NULL, 4);
        case Type::kTrue: return str_dup(LITERAL_TRUE, 4);
        case Type::kFalse: return str_dup(LITERAL_FALSE, 5);
        case Type::kNumber: return stringify_number();
        case Type::kString: return stringify_string();
        case Type::kArray: return stringify_array();
        case Type::kObject: return stringify_object();
        default: break;
    }
    return nullptr;
}

}  // namespace zjson
