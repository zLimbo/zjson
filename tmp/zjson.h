#ifndef ZJSON_H
#define ZJSON_H

#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace zjson {

enum class Type { kNull, kTrue, kFalse, kNumber, kString, kArray, kObject };

enum class Ret {
    kParseOk,
    kParseInvalidValue,
    kParseExpectValue,
    kParseRootNotSingular,
    kParseNumberTooBig,
    kParseMissQuotationMark,
    kParseInvalidStringEscape,
    kParseInvalidStringChar,
    kParseInvalidUnicodeHex,
    kParseInvalidUnicodeSurrogate,
    kParseMissCommaOrSquareBracket,
    kParseMissKey,
    kParseMissColon,
    kParseMissCommaOrCurlyBracket
};

class Json {
public:
    Json();
    Json(const Json &other);
    Json(Json &&other) noexcept;
    Json &operator=(const Json &other);
    Json &operator=(Json &&other) noexcept;
    ~Json();
    void clear();

public:
    Ret parse(const char *text);
    char *stringify();

public:
    Type get_type() const { return type_; }
    bool is_null() const { return type_ == Type::kNull; }
    bool is_bool(bool b) const;
    double get_number() const;
    const char *get_string() const;
    Json &get_array_element(size_t idx) const;
    size_t get_array_size() const;
    size_t get_object_size() const;
    size_t get_object_key_length(size_t idx) const;
    const char *get_object_key(size_t idx) const;
    Json &get_object_value(size_t idx) const;

private:
    void copy(const Json &other);
    void move(Json &other);

    inline void stack_push(char ch);
    inline void stack_push(const char *s);

    inline void check_type(Type type, const char *msg) const;

    Ret parse_text(const char *&text);
    void parse_whitespace(const char *&text);
    Ret parse_literal(const char *&text, std::string_view literal_value,
                      Type type);
    Ret parse_number(const char *&text);
    int parse_hex4(const char *&text);
    Ret encode_utf8(const char *&text);
    Ret parse_string_raw(const char *&text);
    Ret parse_string(const char *&text);
    Ret parse_array(const char *&text);
    Ret parse_object(const char *&text);

    char *stringify_number();
    void stringify_hex4(int code);
    int stringify_utf8(const char *str);
    char *stringify_string_raw(const char *str, int len);
    char *stringify_string();
    char *stringify_array();
    char *stringify_object();

private:
    inline static char *str_dup(const char *str, size_t len);

private:
    Type type_;

    union {
        double number;
        std::string *str;
        std::vector<Json> *array;
        std::vector<std::pair<std::string, Json>> *object;
        // std::multimap<std::string, Json> *object;
    } value_;

    mutable std::vector<char> stack_;
};

Json parse(const char *text);

}  // namespace zjson

#endif  // ZJSON_H