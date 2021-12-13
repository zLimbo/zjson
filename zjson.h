#ifndef ZJSON_H
#define ZJSON_H

#include <string_view>

namespace zjson {

enum Type { TYPE_UNKNOW, TYPE_NULL, TYPE_TRUE, TYPE_FALSE };
enum Ret { PARSE_OK, PARSE_FAIL, PARSE_ROOT_IS_EMPTY, PARSE_ROOT_NOT_SINGULAR };

constexpr std::string_view LITERAL_NULL = "null";
constexpr std::string_view LITERAL_TRUE = "true";
constexpr std::string_view LITERAL_FALSE = "false";

class Json {
public:
public:
    Json() : type_(TYPE_UNKNOW) {}

public:
    Ret parse(std::string_view sv);
    void parse_whitespace(std::string_view &sv);
    Ret parse_literal(std::string_view &sv, std::string_view &literal_value,
                      Type &type);
    Ret parse_null(std::string_view &sv);
    Ret parse_true(std::string_view &sv);
    Ret parse_false(std::string_view &sv);
    Type get_type() const { return type_; }

private:
    Type type_;
};

Json parse(std::string_view sv);

}  // namespace zjson

#endif  // ZJSON_H