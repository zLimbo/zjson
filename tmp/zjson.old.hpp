#ifndef ZJSON_H
#define ZJSON_H

#include <memory.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace zjson {

enum class Type { kNull, kTrue, kFalse, kNumber, kString, kArray, kObject };

using Boolean = bool;
using Number = double;
using String = std::string;
using Array = std::vector<Json>;
using Object = std::map<String, Json>;

union Value {
    Boolean boolean;
    Number number;
    String *str;
    Array *array;
    Object *object;
};

constexpr std::string_view kLiteralNull = "null";
constexpr std::string_view kLiteralTrue = "true";
constexpr std::string_view kLiteralFalse = "false";

class Parser;

class Json {
    friend Parser;

public:
public:
    Json();
    Json(const Json &other);
    Json(Json &&other) noexcept;
    Json &operator=(const Json &other);
    Json &operator=(Json &&other) noexcept;

    Json(double num);
    Json &operator=(double num);
    Json(std::string_view text);
    Json &operator=(std::string_view text);

    Type getType() const { return type_; }
    template <typename T>
    T &getValue();
    Json &operator[](size_t idx);
    Json &operator[](std::string_view key);

private:
    void clear();
    void copy(const Json &other);
    void move(Json &other) noexcept;

public:
    static Json parse(std::string_view text);

private:
private:
    Type type_;
    Value value_;
};

inline void Json::clear() {
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
}

inline void Json::copy(const Json &other) {
    type_ = other.type_;
    switch (type_) {
        case Type::kNumber: value_.number = other.value_.number; break;
        case Type::kString:
            value_.str = new std::string(*other.value_.str);
            break;
        case Type::kArray: value_.array = new Array(*other.value_.array); break;
        case Type::kObject:
            value_.object = new Object(*other.value_.object);
            break;
        default: break;
    }
}

inline void Json::move(Json &other) {
    type_ = other.type_;
    memcpy(&value_, &other.value_, sizeof(value_));
    memset(&other.value_, 0, sizeof(other.value_));
}

inline Json::Json() : type_(Type::kNull) { memset(&value_, 0, sizeof(value_)); }

inline Json::Json(const Json &other) { copy(other); }

inline Json::Json(Json &&other) noexcept { move(other); }

inline Json &Json::operator=(const Json &other) {
    if (this != &other) {
        clear();
        copy(other);
    }
    return *this;
}

inline Json &Json::operator=(Json &&other) noexcept {
    if (this != &other) {
        clear();
        move(other);
    }
    return *this;
}

inline Json::~Json() { clear(); }

class Parser {
public:
    enum class Ret {
        kOk,
        kInvalidValue,
        kExpectValue,
        kRootNotSingular,
        kNumberTooBig,
        kMissQuotationMark,
        kInvalidStringEscape,
        kInvalidStringChar,
        kInvalidUnicodeHex,
        kInvalidUnicodeSurrogate,
        kMissCommaOrSquareBracket,
        kMissKey,
        kMissColon,
        kMissCommaOrCurlyBracket
    };

public:
    static Json parse(std::string_view text, bool allowAccess = false);

private:
    struct Context {
        const char *cur;
        const char *end;
        Json json;
        std::vector<char> stk;
    };

    static void stack_push(Context &ctx, char ch);
    static void stack_push(Context &ctx, std::string_view str);

    static void check_type(Context &ctx, Type type, const char *msg);

    static Parser::Ret parse_text(Context &ctx);
    static void parse_whitespace(Context &ctx);
    static Parser::Ret parse_literal(Context &ctx,
                                     std::string_view literal_value, Type type);
    static Parser::Ret parse_number(Context &ctx);
    static int ch2hex(char ch);
    static int parse_hex4(Context &ctx);
    static Parser::Ret encode_utf8(Context &ctx);
    static Parser::Ret parse_string_raw(Context &ctx);
    static Parser::Ret parse_string(Context &ctx);
    static Parser::Ret parse_array(Context &ctx);
    static Parser::Ret parse_object(Context &ctx);

private:
};

inline Json Parser::parse(std::string_view text, bool allowAccess = false) {
    Context ctx;
    ctx.cur = text.data();
    ctx.end = ctx.cur + text.size();

    Parser::Parser::Ret ret = parse_text(ctx);

    if (ret != Parser::Ret::kOk) {
        throw std::runtime_error("parse failed!");
    }

    if (!allowAccess) {
        parse_whitespace(ctx);
        if (ctx.cur != ctx.end) {
            throw std::runtime_error("kRootNotSingular!");
        }
    }

    return std::move(ctx.json);
}

inline void Parser::stack_push(Context &ctx, char ch) { ctx.stk.push_back(ch); }

inline void Parser::stack_push(Context &ctx, std::string_view str) {
    for (char ch : str) {
        ctx.stk.push_back(ch);
    }
}

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
inline void Parser::parse_whitespace(Context &ctx) {
    while (ctx.cur != ctx.end && (*ctx.cur == ' ' || *ctx.cur == '\t' ||
                                  *ctx.cur == '\n' || *ctx.cur == '\r')) {
        ++ctx.cur;
    }
}

inline Parser::Ret Parser::parse_text(Context &ctx) {
    parse_whitespace(ctx);
    if (ctx.cur == ctx.end) return Ret::kExpectValue;
    switch (*ctx.cur) {
        case 'n': return parse_literal(ctx, kLiteralNull, Type::kNull);
        case 't': return parse_literal(ctx, kLiteralTrue, Type::kTrue);
        case 'f': return parse_literal(ctx, kLiteralFalse, Type::kFalse);
        case '\"': return parse_string(ctx);
        case '[': return parse_array(ctx);
        case '{': return parse_object(ctx);
        default: return parse_number(ctx);
    }
    return Ret::kInvalidValue;
}

inline void Parser::check_type(Context &ctx, Type type, const char *msg) {
    if (ctx.json.type_ != type) {
        std::string error_msg = std::string("ctx value isn't' ") + msg + "!";
        throw std::runtime_error(error_msg);
    }
}

inline Parser::Ret Parser::parse_literal(Context &ctx, std::string_view literal,
                                         Type type) {
    for (char c : literal) {
        if (ctx.cur == ctx.end || *ctx.cur++ != c) return Ret::kInvalidValue;
    }
    ctx.json.type_ = type;
    return Ret::kOk;
}

inline Parser::Ret Parser::parse_number(Context &ctx) {
    // TODO 暂时使用系统库的解析方式匹配测试用例
    // 检查
    const char *p = ctx.cur;
    if (*p == '-') ++p;
    if (*p == '0') {
        ++p;
    } else if (isdigit(*p)) {
        ++p;
        while (isdigit(*p)) ++p;
    } else {
        return Ret::kInvalidValue;
    }
    if (*p == '.') {
        ++p;
        if (!isdigit(*p)) return Ret::kInvalidValue;
        ++p;
        while (isdigit(*p)) ++p;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (!isdigit(*p)) return Ret::kInvalidValue;
        ++p;
        while (isdigit(*p)) ++p;
    }

    double number = strtod(ctx.cur, nullptr);
    if (std::isinf(number)) return Ret::kNumberTooBig;

    ctx.cur = p;
    ctx.json.type_ = Type::kNumber;
    ctx.json.value_.number = number;
    return Ret::kOk;
}

inline int Parser::ch2hex(char ch) {
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

inline int Parser::parse_hex4(Context &ctx) {
    int code = 0;
    for (int i = 0; i < 4; ++i) {
        int hex = ch2hex(*ctx.cur++);
        if (hex < 0) return -1;
        code = (code << 4) + hex;
    }
    return code;
}

inline Parser::Ret Parser::encode_utf8(Context &ctx) {
    int code = parse_hex4(ctx);
    if (code < 0) return Ret::kInvalidUnicodeHex;
    if (code >= 0xD800 && code < 0xDC00) {
        if (*ctx.cur++ != '\\' || *ctx.cur++ != 'u') {
            return Ret::kInvalidUnicodeSurrogate;
        }
        int surrogate = parse_hex4(ctx);
        if (surrogate < 0 || !(surrogate >= 0xDC00 && surrogate < 0xE000)) {
            return Ret::kInvalidUnicodeSurrogate;
        }
        code = 0x10000 + (code - 0xD800) * 0x400 + (surrogate - 0xDC00);
    }

    if (code < 0x80) {
        stack_push(ctx, code);
    } else if (code < 0x800) {
        stack_push(ctx, 0xC0 | (0x1F & (code >> 6)));
        stack_push(ctx, 0X80 | (0x3F & code));
    } else if (code < 0x10000) {
        stack_push(ctx, 0xE0 | (0x0F & (code >> 12)));
        stack_push(ctx, 0X80 | (0x3F & (code >> 6)));
        stack_push(ctx, 0X80 | (0x3F & code));
    } else if (code < 0x10FFFF) {
        stack_push(ctx, 0xF0 | (0x07 & (code >> 18)));
        stack_push(ctx, 0X80 | (0x3F & (code >> 12)));
        stack_push(ctx, 0X80 | (0x3F & (code >> 6)));
        stack_push(ctx, 0X80 | (0x3F & code));
    } else {
        return Ret::kInvalidUnicodeHex;
    }
    return Ret::kOk;
}

Parser::Ret Parser::parse_string_raw(Context &ctx) {
    ++ctx.cur;
    while (*ctx.cur && *ctx.cur != '\"') {
        unsigned char ch = *ctx.cur++;
        if (ch == '\\') {
            switch (*ctx.cur++) {
                case 'b': stack_push(ctx, '\b'); break;
                case 'f': stack_push(ctx, '\f'); break;
                case 'n': stack_push(ctx, '\n'); break;
                case 'r': stack_push(ctx, '\r'); break;
                case 't': stack_push(ctx, '\t'); break;
                case '/': stack_push(ctx, '/'); break;
                case '\"': stack_push(ctx, '\"'); break;
                case '\\': stack_push(ctx, '\\'); break;
                case 'u': {
                    Parser::Ret ret = encode_utf8(ctx);
                    if (ret != Ret::kOk) return ret;
                } break;
                default: return Ret::kInvalidStringEscape;
            }
        } else if (ch < 0x20) {
            return Ret::kInvalidStringChar;
        } else {
            stack_push(ctx, ch);
        }
    }
    if (*ctx.cur++ != '\"') return Ret::kMissQuotationMark;
    return Ret::kOk;
}

inline Parser::Ret Parser::parse_string(Context &ctx) {
    size_t old_top = ctx.stk.size();
    Parser::Ret ret = parse_string_raw(ctx);
    if (ret != Ret::kOk) return ret;
    ctx.json.value_.str =
        new std::string(ctx.stk.data() + old_top, ctx.stk.size() - old_top);
    ctx.stk.resize(old_top);
    ctx.json.type_ = Type::kString;
    return Ret::kOk;
}

Parser::Ret Parser::parse_array(Context &ctx) {
    ++ctx.cur;
    parse_whitespace(ctx);
    if (ctx.cur == ctx.end) {
        return Ret::kMissCommaOrSquareBracket;
    } else if (*ctx.cur == ']') {
        ++ctx.cur;
        ctx.json.value_.array = new Array();
        ctx.json.type_ = Type::kArray;
        return Ret::kOk;
    }

    Array array;
    for (;;) {
        Context subCtx;
        subCtx.cur = ctx.cur;
        subCtx.end = ctx.end;
        Parser::Ret ret = parse_text(subCtx);
        if (ret != Ret::kOk) return ret;
        array.emplace_back(std::move(subCtx.json));

        parse_whitespace(ctx);
        if (*ctx.cur == ']') {
            ++ctx.cur;
            break;
        } else if (*ctx.cur != ',') {
            return Ret::kMissCommaOrSquareBracket;
        }
        ++ctx.cur;
    }
    ctx.json.value_.array = new Array(std::move(array));
    ctx.json.type_ = Type::kArray;
    return Ret::kOk;
}

Parser::Ret Parser::parse_object(Context &ctx) {
    ++ctx.cur;
    parse_whitespace(ctx);
    if (ctx.cur == ctx.end) {
        return Ret::kMissCommaOrCurlyBracket;
    } else if (*ctx.cur == '}') {
        ++ctx.cur;
        ctx.json.value_.object = new Object();
        ctx.json.type_ = Type::kObject;
        return Ret::kOk;
    }

    std::vector<std::pair<std::string, Json>> object;
    for (;;) {
        int old_top = ctx.stk.size();
        if (*ctx.cur != '\"' || parse_string_raw(ctx) != Ret::kOk) {
            return Ret::kMissKey;
        }
        parse_whitespace(ctx);
        if (*ctx.cur++ != ':') return Ret::kMissColon;

        std::string key(ctx.stk.data() + old_top, ctx.stk.size() - old_top);
        ctx.stk.resize(old_top);

        Json value;
        Parser::Ret ret = value.parse_text(ctx);
        if (ret != Ret::kOk) return ret;

        object.emplace_back(std::move(key), std::move(value));

        parse_whitespace(ctx);
        if (*ctx.cur == '}') {
            ++ctx.cur;
            break;
        } else if (*ctx.cur != ',') {
            return Ret::kMissCommaOrCurlyBracket;
        }
        ++ctx.cur;
        parse_whitespace(ctx);
    }

    ctx.json.value_.object =
        new std::vector<std::pair<std::string, Json>>(std::move(object));
    ctx.json.type_ = Type::kObject;
    return Ret::kOk;
}

}  // namespace zjson

#endif  // ZJSON_H