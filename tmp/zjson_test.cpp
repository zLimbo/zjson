
#include "zjson.h"
#define FMT_HEADER_ONLY
#include <fmt/printf.h>

#include <cstdio>

using namespace zjson;

static int test_count = 0;
static int test_pass = 0;
static int main_ret = 0;

#define EXPECT_EQ_BASE(equality, expect, actual)                               \
    do {                                                                       \
        ++test_count;                                                          \
        if (equality) {                                                        \
            ++test_pass;                                                       \
        } else {                                                               \
            fmt::print(stderr, "{}:{}: expect: [{}] actual: [{}]\n", __FILE__, \
                       __LINE__, expect, actual);                              \
            main_ret = 1;                                                      \
        }                                                                      \
    } while (0)

#define EXPECT_EQ(expect, actual) \
    EXPECT_EQ_BASE((expect) == (actual), expect, actual)

#define EXPECT_EQ_STRING(expect, actual) \
    EXPECT_EQ(std::string_view(expect), std::string_view(actual))

#define TEST_PARSE(expect, type, json)        \
    do {                                      \
        Json value;                           \
        EXPECT_EQ(expect, value.parse(json)); \
        EXPECT_EQ(type, value.get_type());    \
    } while (0)

#define TEST_ERROR(error, json) TEST_PARSE(error, Type::kNull, json)
#define TEST_PARSE_SUCCESS(type, json) TEST_PARSE(Ret::kParseOk, type, json)

#define TEST_PARSE_NULL(json) TEST_PARSE_SUCCESS(Type::kNull, json)
#define TEST_PARSE_TRUE(json) TEST_PARSE_SUCCESS(Type::kTrue, json)
#define TEST_PARSE_FALSE(json) TEST_PARSE_SUCCESS(Type::kFalse, json)

static void test_parse_null() {
    TEST_PARSE_NULL("null");
    TEST_PARSE_NULL("   null");
    TEST_PARSE_NULL("null    ");
}

static void test_parse_true() {
    TEST_PARSE_TRUE("true");
    TEST_PARSE_TRUE("   true");
    TEST_PARSE_TRUE("true    ");
}

static void test_parse_false() {
    TEST_PARSE_FALSE("false");
    TEST_PARSE_FALSE("   false");
    TEST_PARSE_FALSE("false    ");
}

static void test_parse_expect_value() {
    TEST_ERROR(Ret::kParseExpectValue, "");
    TEST_ERROR(Ret::kParseExpectValue, " \t\n \r\t   ");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(Ret::kParseRootNotSingular, "null x");
    TEST_ERROR(Ret::kParseRootNotSingular, "   false true ");
    TEST_ERROR(Ret::kParseRootNotSingular, "   true false");

    TEST_ERROR(Ret::kParseRootNotSingular,
               "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(Ret::kParseRootNotSingular, "0x0");
    TEST_ERROR(Ret::kParseRootNotSingular, "0x123");
}

#define TEST_NUMBER(expect, json)                    \
    do {                                             \
        Json value;                                  \
        EXPECT_EQ(Ret::kParseOk, value.parse(json)); \
        EXPECT_EQ(Type::kNumber, value.get_type());        \
        EXPECT_EQ(expect, value.get_number());       \
    } while (0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002,
                "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324,
                "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308,
                "2.2250738585072009e-308"); /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308,
                "2.2250738585072014e-308"); /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308,
                "1.7976931348623157e+308"); /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_invalid_value() {
    /* invalid null */
    TEST_ERROR(Ret::kParseInvalidValue, "   n ull   ");
    /* invalid true */
    TEST_ERROR(Ret::kParseInvalidValue, "   tr ue   ");
    /* invalid false */
    TEST_ERROR(Ret::kParseInvalidValue, "   fals   ");

    /* invalid number */
    TEST_ERROR(Ret::kParseInvalidValue, "+0");
    TEST_ERROR(Ret::kParseInvalidValue, "+1");
    TEST_ERROR(Ret::kParseInvalidValue,
               ".123"); /* at least one digit before '.' */
    TEST_ERROR(Ret::kParseInvalidValue,
               "1."); /* at least one digit after '.' */
    TEST_ERROR(Ret::kParseInvalidValue, "INF");
    TEST_ERROR(Ret::kParseInvalidValue, "inf");
    TEST_ERROR(Ret::kParseInvalidValue, "NAN");
    TEST_ERROR(Ret::kParseInvalidValue, "nan");
}

static void test_parse_number_too_big() {
    TEST_ERROR(Ret::kParseNumberTooBig, "1e309");
    TEST_ERROR(Ret::kParseNumberTooBig, "-1e309");
}

#define TEST_STRING(expect, json)                        \
    do {                                                 \
        Json value;                                      \
        EXPECT_EQ(Ret::kParseOk, value.parse(json));     \
        EXPECT_EQ(Type::kString, value.get_type());            \
        EXPECT_EQ(std::string_view(expect),              \
                  std::string_view(value.get_string())); \
    } while (0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t",
                "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E",
                "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E",
                "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

static void test_parse_miss_quotation_mark() {
    TEST_ERROR(Ret::kParseMissQuotationMark, "\"");
    TEST_ERROR(Ret::kParseMissQuotationMark, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(Ret::kParseInvalidStringEscape, "\"\\V\"");
    TEST_ERROR(Ret::kParseInvalidStringEscape, "\"\\'\"");
    TEST_ERROR(Ret::kParseInvalidStringEscape, "\"\\0\"");
    TEST_ERROR(Ret::kParseInvalidStringEscape, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(Ret::kParseInvalidStringChar, "\"\x01\"");
    TEST_ERROR(Ret::kParseInvalidStringChar, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u0\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u01\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u012\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u/000\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\uG000\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u0/00\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u0G00\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u00/0\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u00G0\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u000/\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u000G\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeHex, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(Ret::kParseInvalidUnicodeSurrogate, "\"\\uD800\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeSurrogate, "\"\\uDBFF\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeSurrogate, "\"\\uD800\\\\\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeSurrogate, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(Ret::kParseInvalidUnicodeSurrogate, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
    size_t i, j;
    Json v;

    EXPECT_EQ(Ret::kParseOk, v.parse("[ ]"));
    EXPECT_EQ(Type::kArray, v.get_type());
    EXPECT_EQ(0, v.get_array_size());

    v.clear();

    EXPECT_EQ(Ret::kParseOk,
              v.parse("[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ(Type::kArray, v.get_type());
    EXPECT_EQ(5, v.get_array_size());
    EXPECT_EQ(Type::kNull, v.get_array_element(0).get_type());
    EXPECT_EQ(Type::kFalse, v.get_array_element(1).get_type());
    EXPECT_EQ(Type::kTrue, v.get_array_element(2).get_type());
    EXPECT_EQ(Type::kNumber, v.get_array_element(3).get_type());
    EXPECT_EQ(Type::kString, v.get_array_element(4).get_type());
    EXPECT_EQ(123.0, v.get_array_element(3).get_number());
    EXPECT_EQ(std::string_view("abc"), v.get_array_element(4).get_string());

    v.clear();

    EXPECT_EQ(Ret::kParseOk,
              v.parse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ(Type::kArray, v.get_type());
    EXPECT_EQ(4, v.get_array_size());
    for (i = 0; i < 4; i++) {
        const Json& a = v.get_array_element(i);
        EXPECT_EQ(Type::kArray, a.get_type());
        EXPECT_EQ(i, a.get_array_size());
        for (j = 0; j < i; j++) {
            const Json& e = a.get_array_element(j);
            EXPECT_EQ(Type::kNumber, e.get_type());
            EXPECT_EQ((double)j, e.get_number());
        }
    }
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(Ret::kParseMissCommaOrSquareBracket, "[1");
    TEST_ERROR(Ret::kParseMissCommaOrSquareBracket, "[1}");
    TEST_ERROR(Ret::kParseMissCommaOrSquareBracket, "[1 2");
    TEST_ERROR(Ret::kParseMissCommaOrSquareBracket, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(Ret::kParseMissKey, "{:1,");
    TEST_ERROR(Ret::kParseMissKey, "{1:1,");
    TEST_ERROR(Ret::kParseMissKey, "{true:1,");
    TEST_ERROR(Ret::kParseMissKey, "{false:1,");
    TEST_ERROR(Ret::kParseMissKey, "{null:1,");
    TEST_ERROR(Ret::kParseMissKey, "{[]:1,");
    TEST_ERROR(Ret::kParseMissKey, "{{}:1,");
    TEST_ERROR(Ret::kParseMissKey, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(Ret::kParseMissColon, "{\"a\"}");
    TEST_ERROR(Ret::kParseMissColon, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(Ret::kParseMissCommaOrCurlyBracket, "{\"a\":1");
    TEST_ERROR(Ret::kParseMissCommaOrCurlyBracket, "{\"a\":1]");
    TEST_ERROR(Ret::kParseMissCommaOrCurlyBracket, "{\"a\":1 \"b\"");
    TEST_ERROR(Ret::kParseMissCommaOrCurlyBracket, "{\"a\":{}");
}

static void test_parse_object() {
    Json v;
    size_t i;

    EXPECT_EQ(Ret::kParseOk, v.parse(" { } "));
    EXPECT_EQ(Type::kObject, v.get_type());
    EXPECT_EQ(0, v.get_object_size());
    v.clear();

    EXPECT_EQ(Ret::kParseOk,
              v.parse(" { "
                      "\"n\" : null , "
                      "\"f\" : false , "
                      "\"t\" : true , "
                      "\"i\" : 123 , "
                      "\"s\" : \"abc\", "
                      "\"a\" : [ 1, 2, 3 ],"
                      "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                      " } "));
    EXPECT_EQ(Type::kObject, v.get_type());
    EXPECT_EQ(7, v.get_object_size());
    EXPECT_EQ_STRING("n", v.get_object_key(0));
    EXPECT_EQ(Type::kNull, v.get_object_value(0).get_type());
    EXPECT_EQ_STRING("f", v.get_object_key(1));
    EXPECT_EQ(Type::kFalse, v.get_object_value(1).get_type());
    EXPECT_EQ_STRING("t", v.get_object_key(2));
    EXPECT_EQ(Type::kTrue, v.get_object_value(2).get_type());
    EXPECT_EQ_STRING("i", v.get_object_key(3));
    EXPECT_EQ(Type::kNumber, v.get_object_value(3).get_type());
    EXPECT_EQ(123.0, v.get_object_value(3).get_number());
    EXPECT_EQ_STRING("s", v.get_object_key(4));
    EXPECT_EQ(Type::kString, v.get_object_value(4).get_type());
    EXPECT_EQ_STRING("abc", v.get_object_value(4).get_string());
    EXPECT_EQ_STRING("a", v.get_object_key(5));
    EXPECT_EQ(Type::kArray, v.get_object_value(5).get_type());
    EXPECT_EQ(3, v.get_object_value(5).get_array_size());
    for (i = 0; i < 3; i++) {
        Json& e = v.get_object_value(5).get_array_element(i);
        EXPECT_EQ(Type::kNumber, e.get_type());
        EXPECT_EQ(i + 1.0, e.get_number());
    }
    EXPECT_EQ_STRING("o", v.get_object_key(6));
    {
        Json& o = v.get_object_value(6);
        EXPECT_EQ(Type::kObject, o.get_type());
        for (i = 0; i < 3; i++) {
            Json& ov = o.get_object_value(i);
            EXPECT_EQ(char('1' + i), o.get_object_key(i)[0]);
            EXPECT_EQ(1, o.get_object_key_length(i));
            EXPECT_EQ(Type::kNumber, ov.get_type());
            EXPECT_EQ(i + 1.0, ov.get_number());
        }
    }
    v.clear();
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

#define TEST_ROUNDTRIP(json)                         \
    do {                                             \
        Json value;                                  \
        EXPECT_EQ(Ret::kParseOk, value.parse(json)); \
        char* json2 = value.stringify();             \
        EXPECT_EQ_STRING(json, json2);               \
        free(json2);                                 \
    } while (0)

static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002");      /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308"); /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308"); /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308"); /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");

    // TEST_ROUNDTRIP("\"\\u00A2\"");        /* Cents sign U+00A2 */
    // TEST_ROUNDTRIP("\"\\u20AC\"");        /* Euro sign U+20AC */
    // TEST_ROUNDTRIP("\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
    // TEST_ROUNDTRIP("\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP(
        "{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,"
        "3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

int main() {
    test_parse();
    test_stringify();
    // test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
           100.0 * test_pass / test_count);
    return main_ret;
}