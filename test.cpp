
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

static void test_parse_null() {
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("null"));
        EXPECT_EQ_INT(zjson::TYPE_NULL, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   null"));
        EXPECT_EQ_INT(zjson::TYPE_NULL, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   null   "));
        EXPECT_EQ_INT(zjson::TYPE_NULL, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_FAIL, value.parse("   n ull   "));
        EXPECT_EQ_INT(zjson::TYPE_UNKNOW, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_ROOT_NOT_SINGULAR,
                      value.parse("   null   AA"));
        EXPECT_EQ_INT(zjson::TYPE_NULL, value.get_type());
    }
}

static void test_parse_true() {
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("true"));
        EXPECT_EQ_INT(zjson::TYPE_TRUE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   true"));
        EXPECT_EQ_INT(zjson::TYPE_TRUE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   true   "));
        EXPECT_EQ_INT(zjson::TYPE_TRUE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_FAIL, value.parse("   t"));
        EXPECT_EQ_INT(zjson::TYPE_UNKNOW, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_ROOT_NOT_SINGULAR,
                      value.parse("   true   AA"));
        EXPECT_EQ_INT(zjson::TYPE_TRUE, value.get_type());
    }
}

static void test_parse_false() {
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("false"));
        EXPECT_EQ_INT(zjson::TYPE_FALSE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   false"));
        EXPECT_EQ_INT(zjson::TYPE_FALSE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_OK, value.parse("   false   "));
        EXPECT_EQ_INT(zjson::TYPE_FALSE, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_FAIL, value.parse("   f alse   "));
        EXPECT_EQ_INT(zjson::TYPE_UNKNOW, value.get_type());
    }
    {
        zjson::Json value;
        EXPECT_EQ_INT(zjson::PARSE_ROOT_NOT_SINGULAR,
                      value.parse("   false   true"));
        EXPECT_EQ_INT(zjson::TYPE_FALSE, value.get_type());
    }
}

static void test_parse_empty() {
    zjson::Json value;
    EXPECT_EQ_INT(zjson::PARSE_ROOT_IS_EMPTY, value.parse(""));
    EXPECT_EQ_INT(zjson::PARSE_ROOT_IS_EMPTY, value.parse(" \t\n \r\t   "));
}

static void test_parse() {
    test_parse_null();
    test_parse_false();
    test_parse_true();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
           100.0 * test_pass / test_count);
    return main_ret;
}