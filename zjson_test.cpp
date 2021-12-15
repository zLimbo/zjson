
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

#define TEST_PARSE(expect, type, json)        \
    do {                                      \
        Json value;                           \
        EXPECT_EQ(expect, value.parse(json)); \
        EXPECT_EQ(type, value.get_type());    \
    } while (0)

#define TEST_ERROR(error, json) TEST_PARSE(error, TYPE_NULL, json)
#define TEST_PARSE_SUCCESS(type, json) TEST_PARSE(PARSE_OK, type, json)

#define TEST_PARSE_NULL(json) TEST_PARSE_SUCCESS(TYPE_NULL, json)
#define TEST_PARSE_TRUE(json) TEST_PARSE_SUCCESS(TYPE_TRUE, json)
#define TEST_PARSE_FALSE(json) TEST_PARSE_SUCCESS(TYPE_FALSE, json)

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
    TEST_ERROR(PARSE_EXPECT_VALUE, "");
    TEST_ERROR(PARSE_EXPECT_VALUE, " \t\n \r\t   ");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null x");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "   false true ");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "   true false");

    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR,
               "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x123");
}

#define TEST_NUMBER(expect, json)                 \
    do {                                          \
        Json value;                               \
        EXPECT_EQ(PARSE_OK, value.parse(json));   \
        EXPECT_EQ(TYPE_NUMBER, value.get_type()); \
        EXPECT_EQ(expect, value.get_number());    \
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
    TEST_ERROR(PARSE_INVALID_VALUE, "   n ull   ");
    /* invalid true */
    TEST_ERROR(PARSE_INVALID_VALUE, "   tr ue   ");
    /* invalid false */
    TEST_ERROR(PARSE_INVALID_VALUE, "   fals   ");

    /* invalid number */
    TEST_ERROR(PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, "nan");
}

static void test_parse_number_too_big() {
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "-1e309");
}

#define TEST_STRING(expect, json)                        \
    do {                                                 \
        Json value;                                      \
        EXPECT_EQ(PARSE_OK, value.parse(json));          \
        EXPECT_EQ(TYPE_STRING, value.get_type());        \
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

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\V\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
    size_t i, j;
    Json v;

    EXPECT_EQ(PARSE_OK, v.parse("[ ]"));
    EXPECT_EQ(TYPE_ARRAY, v.get_type());
    EXPECT_EQ(0, v.get_array().size());

    v.clear();

    EXPECT_EQ(PARSE_OK, v.parse("[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ(TYPE_ARRAY, v.get_type());
    EXPECT_EQ(5, v.get_array().size());
    EXPECT_EQ(NULL, v.get_array_element(0).get_type());
    EXPECT_EQ(TYPE_FALSE, v.get_array_element(1).get_type());
    EXPECT_EQ(TYPE_TRUE, v.get_array_element(2).get_type());
    EXPECT_EQ(TYPE_NUMBER, v.get_array_element(3).get_type());
    EXPECT_EQ(TYPE_STRING, v.get_array_element(4).get_type());
    EXPECT_EQ(123.0, v.get_array_element(3).get_number());
    EXPECT_EQ("abc", v.get_array_element(4).get_string());

    v.clear();

    EXPECT_EQ(PARSE_OK, v.parse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ(TYPE_ARRAY, v.get_type());
    EXPECT_EQ(4, v.get_array().size());
    for (i = 0; i < 4; i++) {
        const Json& a = v.get_array_element(i);
        EXPECT_EQ(TYPE_ARRAY, a.get_type());
        EXPECT_EQ(i, a.get_array().size());
        for (j = 0; j < i; j++) {
            const Json& e = a.get_array_element(j);
            EXPECT_EQ(TYPE_NUMBER, e.get_type());
            EXPECT_EQ((double)j, e.get_number());
        }
    }
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_object() {
    Json v;
    size_t i;

    // EXPECT_EQ(PARSE_OK, v.parse(" { } "));
    // EXPECT_EQ(OBJECT, v.get_type());
    // EXPECT_EQ(0, v.get_object_size());
    // v.clear()

    //     EXPECT_EQ(PARSE_OK,
    //               v.parse(" { "
    //                       "\"n\" : null , "
    //                       "\"f\" : false , "
    //                       "\"t\" : true , "
    //                       "\"i\" : 123 , "
    //                       "\"s\" : \"abc\", "
    //                       "\"a\" : [ 1, 2, 3 ],"
    //                       "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
    //                       " } "));
    // EXPECT_EQ(OBJECT, v.get_type());
    // EXPECT_EQ(7, v.get_object_size());
    // EXPECT_EQ("n", v.get_object_key(0), v.get_object_key_length(0));
    // EXPECT_EQ(NULL, v.get_object_value(0).get_type());
    // EXPECT_EQ("f", v.get_object_key(1), v.get_object_key_length(1));
    // EXPECT_EQ(FALSE, v.get_object_value(1).get_type());
    // EXPECT_EQ("t", v.get_object_key(2), v.get_object_key_length(2));
    // EXPECT_EQ(TRUE, v.get_object_value(2).get_type());
    // EXPECT_EQ("i", v.get_object_key(3), v.get_object_key_length(3));
    // EXPECT_EQ(NUMBER, v.get_object_value(3).get_type());
    // EXPECT_EQ(123.0, get_number(v.get_object_value(3)));
    // EXPECT_EQ("s", v.get_object_key(4), v.get_object_key_length(4));
    // EXPECT_EQ(STRING, v.get_object_value(4).get_type());
    // EXPECT_EQ("abc", get_string(v.get_object_value(4)),
    //           get_string_length(v.get_object_value(4)));
    // EXPECT_EQ("a", v.get_object_key(5), v.get_object_key_length(5));
    // EXPECT_EQ(ARRAY, v.get_object_value(5).get_type());
    // EXPECT_EQ(3, get_array_size(v.get_object_value(5)));
    // for (i = 0; i < 3; i++) {
    //     value* e = get_array_element(v.get_object_value(5), i);
    //     EXPECT_EQ(NUMBER, get_type(e));
    //     EXPECT_EQ(i + 1.0, get_number(e));
    // }
    // EXPECT_EQ("o", v.get_object_key(6), v.get_object_key_length(6));
    // {
    //     value* o = v.get_object_value(6);
    //     EXPECT_EQ(OBJECT, get_type(o));
    //     for (i = 0; i < 3; i++) {
    //         value* ov = get_object_value(o, i);
    //         EXPECT_TRUE('1' + i == get_object_key(o, i)[0]);
    //         EXPECT_EQ(1, get_object_key_length(o, i));
    //         EXPECT_EQ(NUMBER, get_type(ov));
    //         EXPECT_EQ(i + 1.0, get_number(ov));
    //     }
    // }
    // v.clear()
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
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
           100.0 * test_pass / test_count);
    return main_ret;
}