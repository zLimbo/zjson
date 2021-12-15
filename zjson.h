#ifndef ZJSON_H
#define ZJSON_H

#include <string>
#include <string_view>
#include <tuple>

namespace zjson {

enum Type { TYPE_NULL, TYPE_TRUE, TYPE_FALSE, TYPE_NUMBER, TYPE_STRING };
enum Ret {
    PARSE_OK,
    PARSE_INVALID_VALUE,
    PARSE_EXPECT_VALUE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_NUMBER_TOO_BIG,
    PARSE_MISS_QUOTATION_MARK,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_INVALID_STRING_CHAR,
    PARSE_INVALID_UNICODE_HEX,
    PARSE_INVALID_UNICODE_SURROGATE
};

constexpr std::string_view LITERAL_NULL = "null";
constexpr std::string_view LITERAL_TRUE = "true";
constexpr std::string_view LITERAL_FALSE = "false";

class Json {
public:
    Json();
    ~Json();

public:
    Type get_type() const { return type_; }
    void stack_push(char ch) { stack_.push_back(ch); }

public:
    Ret parse(std::string_view sv);
    void parse_whitespace(std::string_view &sv);
    Ret parse_literal(std::string_view &sv, std::string_view literal_value,
                      Type type);
    Ret parse_number(std::string_view &sv);
    int parse_hex4(std::string_view sv);
    std::tuple<Ret, int> encode_utf8(std::string_view sv);
    Ret parse_string(std::string_view &sv);

public:
    double get_number() const;
    const char *get_string() const;

private:
    void clear();

private:
    Type type_;

    union {
        double number;
        char *str;
    } value_;

    std::string stack_;
};

Json parse(std::string_view sv);

}  // namespace zjson

#endif  // ZJSON_H