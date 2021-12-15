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
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    PARSE_MISS_KEY,
    PARSE_MISS_COLON,
    PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

class Json {
public:
    Json();
    Json(const Json &other);
    Json(Json &&other) noexcept;
    Json &operator=(const Json &other);
    Json &operator=(Json &&other);
    ~Json();
    void clear();

public:
    Type get_type() const { return type_; }
    void stack_push(char ch) { stack_.push_back(ch); }

public:
    Ret parse(const char *text);

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

    Ret parse_text(const char *&text);

    void parse_whitespace(const char *&text);

    Ret parse_literal(const char *&text, std::string_view literal_value,
                      Type type);

    Ret parse_number(const char *&text);

    int parse_hex4(const char *&text);
    Ret encode_utf8(const char *&text);
    Ret parse_string(const char *&text);

    Ret parse_array(const char *&text);

private:
    Type type_;

    union {
        double number;
        std::string *str;
        std::vector<Json> *array;
    } value_;

    std::string stack_;
};

Json parse(const char *text);

}  // namespace zjson

#endif  // ZJSON_H