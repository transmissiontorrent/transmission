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

TR_DECLARE_CONVERTER(tr::app::ShowMode)
TR_DECLARE_CONVERTER(tr::app::SortMode)
TR_DECLARE_CONVERTER(tr::app::StatsMode)
TR_DECLARE_CONVERTER(std::chrono::sys_seconds)

} // namespace tr::serializer
