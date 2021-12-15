
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

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
           100.0 * test_pass / test_count);
    return main_ret;
}