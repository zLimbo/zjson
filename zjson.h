#ifndef ZJSON_H
#define ZJSON_H

#include <string_view>

namespace zjson {

enum Type { TYPE_NULL, TYPE_TRUE, TYPE_FALSE, TYPE_NUMBER };
enum Ret {
    PARSE_OK,
    PARSE_INVALID_VALUE,
    PARSE_EXPECT_VALUE,
    PARSE_ROOT_NOT_SINGULAR
};

constexpr std::string_view LITERAL_NULL = "null";
constexpr std::string_view LITERAL_TRUE = "true";
constexpr std::string_view LITERAL_FALSE = "false";

class Json {
public:
public:
    Json() : type_(TYPE_NULL) {}

public:
    Type get_type() const { return type_; }

    Ret parse(std::string_view sv);
    void parse_whitespace(std::string_view &sv);
    Ret parse_literal(std::string_view &sv, std::string_view literal_value,
                      Type type);
    Ret parse_number(std::string_view &sv);
    double get_as_number();

private:
    Type type_;

    union {
        double number;
    } value_;
};

Json parse(std::string_view sv);

}  // namespace zjson

#endif  // ZJSON_H