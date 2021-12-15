#ifndef ZJSON_H
#define ZJSON_H

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace zjson {

enum Type {
    TYPE_NULL,
    TYPE_TRUE,
    TYPE_FALSE,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ARRAY
};
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
    PARSE_INVALID_UNICODE_SURROGATE,
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

constexpr std::string_view LITERAL_NULL = "null";
constexpr std::string_view LITERAL_TRUE = "true";
constexpr std::string_view LITERAL_FALSE = "false";

class Json {
public:
    Json();
    Json(const Json &other);
    Json(Json &&other);
    Json &operator=(const Json &other);
    Json &operator=(Json &&other);
    ~Json();
    void clear();

public:
    Type get_type() const { return type_; }
    void stack_push(char ch) { stack_.push_back(ch); }

public:
    Ret parse(std::string_view text);

public:
    bool is_null() const { return type_ == TYPE_NULL; }
    bool is_bool(bool b) const;
    double get_number() const;
    const std::string &get_string() const;
    const std::vector<Json> &get_array() const;
    const Json &get_array_element(size_t idx) const;

private:
    void copy(const Json &other);
    void move(Json &other);

    Ret parse_text(std::string_view &text);

    void parse_whitespace(std::string_view &text);

    Ret parse_literal(std::string_view &text, std::string_view literal_value,
                      Type type);

    Ret parse_number(std::string_view &text);

    int parse_hex4(std::string_view text);
    std::tuple<Ret, int> encode_utf8(std::string_view text);
    Ret parse_string(std::string_view &text);

    Ret parse_array(std::string_view &text);

private:
    Type type_;

    union {
        double number;
        std::string *str;
        std::vector<Json> *array;
    } value_;

    std::string stack_;
};

Json parse(std::string_view text);

}  // namespace zjson

#endif  // ZJSON_H