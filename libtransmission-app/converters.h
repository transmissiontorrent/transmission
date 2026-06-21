// This file Copyright © Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <chrono>

#include <libtransmission/converters.h>
#include <libtransmission/variant.h>

#include "libtransmission-app/display-modes.h"

// Declarations of the `Converter<T>` specializations owned by
// `libtransmission-app`. Each TU that uses `tr::serializer::to_variant`,
// `to_value`, `Field<>`, etc. with one of these types must include this
// header so the specialization is visible at the instantiation site.

namespace tr::serializer
{
template<>
struct Converter<tr::app::ShowMode>
{
    static tr_variant to_variant(tr::app::ShowMode const& src);
    static bool to_value(tr_variant const& src, tr::app::ShowMode* tgt);
};

template<>
struct Converter<tr::app::SortMode>
{
    static tr_variant to_variant(tr::app::SortMode const& src);
    static bool to_value(tr_variant const& src, tr::app::SortMode* tgt);
};

template<>
struct Converter<tr::app::StatsMode>
{
    static tr_variant to_variant(tr::app::StatsMode const& src);
    static bool to_value(tr_variant const& src, tr::app::StatsMode* tgt);
};

template<>
struct Converter<std::chrono::sys_seconds>
{
    static tr_variant to_variant(std::chrono::sys_seconds const& src);
    static bool to_value(tr_variant const& src, std::chrono::sys_seconds* tgt);
};
} // namespace tr::serializer
