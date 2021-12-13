
#include <cstdio>

#include "zjson.h"

static int test_count = 0;
static int test_pass = 0;
static int main_ret = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                      \
    do {                                                                      \
        ++test_count;                                                         \
        if (equality) {                                                       \
            ++test_pass;                                                      \
        } else {                                                              \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
                    __FILE__, __LINE__, expect, actual);                      \
            main_ret = 1;                                                     \
        }                                                                     \
    } while (0)

#define EXPECT_EQ(expect, actual, format) \
    EXPECT_EQ_BASE((expect) == (actual), expect, actual, format)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ(expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ(expect, actual, "%f")

#define TEST_PARSE(expect, type, json)            \
    do {                                          \
        zjson::Json value;                        \
        EXPECT_EQ_INT(expect, value.parse(json)); \
        EXPECT_EQ_INT(type, value.get_type());    \
    } while (0)

#define TEST_PARSE_ERROR(error, json) TEST_PARSE(error, zjson::TYPE_NULL, json)
#define TEST_PARSE_SUCCESS(type, json) TEST_PARSE(zjson::PARSE_OK, type, json)

#define TEST_PARSE_NULL(json) TEST_PARSE_SUCCESS(zjson::TYPE_NULL, json)
#define TEST_PARSE_TRUE(json) TEST_PARSE_SUCCESS(zjson::TYPE_TRUE, json)
#define TEST_PARSE_FALSE(json) TEST_PARSE_SUCCESS(zjson::TYPE_FALSE, json)

#define TEST_NUMBER(expect, json)                            \
    do {                                                     \
        zjson::Json value;                                   \
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse(json));   \
        EXPECT_EQ_INT(zjson::TYPE_NUMBER, value.get_type()); \
        EXPECT_EQ_DOUBLE(expect, value.get_as_number());     \
    } while (0)

static void test_parse_null() {
    TEST_PARSE_NULL("null");
    TEST_PARSE_NULL("   null");
    TEST_PARSE_NULL("null    ");

    TEST_PARSE(zjson::PARSE_ROOT_NOT_SINGULAR, zjson::TYPE_NULL,
               "   null   AA");
}

static void test_parse_true() {
    TEST_PARSE_TRUE("true");
    TEST_PARSE_TRUE("   true");
    TEST_PARSE_TRUE("true    ");

    TEST_PARSE(zjson::PARSE_ROOT_NOT_SINGULAR, zjson::TYPE_TRUE,
               "   true false");
}

static void test_parse_false() {
    TEST_PARSE_FALSE("false");
    TEST_PARSE_FALSE("   false");
    TEST_PARSE_FALSE("false    ");

    TEST_PARSE(zjson::PARSE_ROOT_NOT_SINGULAR, zjson::TYPE_FALSE,
               "   false true ");
}

static void test_parse_expect_value() {
    TEST_PARSE_ERROR(zjson::PARSE_EXPECT_VALUE, "");
    TEST_PARSE_ERROR(zjson::PARSE_EXPECT_VALUE, " \t\n \r\t   ");
}

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
}

static void test_parse_invalid_value() {
    /* invalid null */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "   n ull   ");
    /* invalid true */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "   tr ue   ");
    /* invalid false */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "   fals   ");

    /* invalid number */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "+0");
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "+1");
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE,
                     ".123"); /* at least one digit before '.' */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE,
                     "1."); /* at least one digit after '.' */
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "INF");
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "inf");
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "NAN");
    TEST_PARSE_ERROR(zjson::PARSE_INVALID_VALUE, "nan");
}

static void test_parse() {
    test_parse_null();
    test_parse_false();
    test_parse_true();
    test_parse_expect_value();
    test_parse_number();
    test_parse_invalid_value();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
           100.0 * test_pass / test_count);
    return main_ret;
}