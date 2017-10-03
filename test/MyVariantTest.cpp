//
// Created by akarpovskii on 22.09.17.
//

#include <gtest/gtest.h>
#include "../include/variant.hpp"

using namespace my_variant;

template<typename T>
struct TestClass {
    TestClass() = default;

    explicit TestClass(const T &t) : t(t) {}

    explicit TestClass(T &&t) : t(std::move(t)) {};

    explicit operator T() const {
        return t;
    }

    ~TestClass() = default;

    bool operator==(const TestClass<T> &that) const {
        return t == that.t;
    }

    T t;
};

TEST(MyVariantTest, SetGet) {
    variant<int, double, TestClass<int>, std::string> var(1);
    EXPECT_EQ(var.get<int>(), 1);
    var = 1.1;
    EXPECT_NEAR(var.get<double>(), 1.1, 1E-6);
    var = std::string("1");
    EXPECT_EQ(var.get<std::string>(), "1");
    TestClass<int> tc(1);
    var = tc;
    EXPECT_EQ(var.get<TestClass<int>>(), tc);
}

TEST(MyVariantTest, HoldsAlternatibe) {
    variant<int, double, TestClass<int>, std::string> var(1);
    EXPECT_TRUE(var.holds_alternative<int>());
    var = 1.1;
    EXPECT_TRUE(var.holds_alternative<double>());
    var = std::string("1");
    EXPECT_TRUE(var.holds_alternative<std::string>());
    var = TestClass<int>();
    EXPECT_TRUE(var.holds_alternative<TestClass<int>>());
}

TEST(MyVariantTest, Index) {
    variant<int, double, TestClass<int>, std::string> var(1);
    EXPECT_EQ(var.index(), 0);
    var = 1.1;
    EXPECT_EQ(var.index(), 1);
    var = std::string("1");
    EXPECT_EQ(var.index(), 3);
    var = TestClass<int>();
    EXPECT_EQ(var.index(), 2);
}

TEST(MyVariantTest, Equality) {
    variant<int, double, TestClass<int>, std::string> var1(1), var2(1);
    EXPECT_EQ(var1, var2);
    var1 = 1.1;
    EXPECT_NE(var1, var2);
    var1 = TestClass(5);
    var2 = TestClass(5);
    EXPECT_EQ(var1, var2);
}

TEST(MyVariantTest, CanConvert) {
    variant<int, double, TestClass<int>, std::string> var(1);
    EXPECT_TRUE(var.can_convert<double>());
    EXPECT_TRUE(var.can_convert<std::string>());
    EXPECT_TRUE(var.can_convert<TestClass<int>>());
    EXPECT_TRUE(var.can_convert<int>());
    var = TestClass(1);
    EXPECT_TRUE(var.can_convert<int>());
    EXPECT_FALSE(var.can_convert<double>());
}

TEST(MyVariantTest, Convert) {
    variant<int, double, TestClass<int>, std::string> var(1);
    EXPECT_NO_THROW(EXPECT_NEAR(var.convert<double>(), 1.0, 1E-6));
    EXPECT_NO_THROW(EXPECT_EQ(var.convert<std::string>(), "1"));
    EXPECT_NO_THROW(EXPECT_EQ(var.convert<TestClass<int>>(), TestClass(1)));
    EXPECT_NO_THROW(EXPECT_EQ(var.convert<int>(), 1));
    var = TestClass(1);
    EXPECT_NO_THROW(EXPECT_EQ(var.convert<int>(), 1));
    EXPECT_THROW(var.convert<double>(), std::bad_cast);
}