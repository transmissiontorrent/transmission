// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#ifndef TR_QT_TEST_QT_TEST_FIXTURES_H
#define TR_QT_TEST_QT_TEST_FIXTURES_H

#include <type_traits>

#include <QTemporaryDir>
#include <QTest>

#include <fmt/format.h>

#include <libtransmission/converters.h>
#include <libtransmission/variant.h>

#include "TrQtInit.h"
#include "VariantHelpers.h"

// QCOMPARE_EQ / QCOMPARE_NE only exist in Qt >= 6.3, but Transmission still
// supports Qt 5.15. These give equivalent comparisons (fuzzy for floats) and,
// on failure, render both operands as JSON via the serializer. Distinct `T`/`U`
// allow mixed operand types; an operand without a Converter is a compile error.
template<typename T, typename U>
void trcompare(T const& actual, U const& expected, bool const negate)
{
    auto ok = bool{};
    if constexpr (std::is_floating_point_v<T> && std::is_floating_point_v<U>) {
        ok = qFuzzyCompare(actual, expected);
    } else {
        ok = actual == expected;
    }

    if (negate ? !ok : ok) {
        return;
    }

    auto serde = tr_variant_serde::json();
    serde.compact();
    auto const actual_str = serde.to_string(tr::serializer::to_variant(actual));
    auto const expected_str = serde.to_string(tr::serializer::to_variant(expected));
    QFAIL(fmt::format("got '{:s}', expected '{:s}'", actual_str, expected_str).c_str());
}

#define TRCOMPARE_EQ(actual, expected) trcompare((actual), (expected), false)
#define TRCOMPARE_NE(actual, expected) trcompare((actual), (expected), true)

class BasicTest
{
public:
    BasicTest()
    {
        trqt::trqt_init();
    }

    BasicTest(BasicTest&&) = delete;
    BasicTest(BasicTest const&) = delete;
    BasicTest& operator=(BasicTest&&) = delete;
    BasicTest& operator=(BasicTest const&) = delete;
    virtual ~BasicTest() = default;
};

class SandboxedTest : public BasicTest
{
public:
    SandboxedTest() = default;
    SandboxedTest(SandboxedTest&&) = delete;
    SandboxedTest(SandboxedTest const&) = delete;
    SandboxedTest& operator=(SandboxedTest&&) = delete;
    SandboxedTest& operator=(SandboxedTest const&) = delete;
    ~SandboxedTest() override = default;

    [[nodiscard]] bool isValid() const
    {
        return sandbox_.isValid();
    }

    [[nodiscard]] QString sandboxDir() const
    {
        return sandbox_.path();
    }

private:
    QTemporaryDir sandbox_{};
};

#endif // TR_QT_TEST_QT_TEST_FIXTURES_H
