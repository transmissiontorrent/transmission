// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <cstddef> // size_t
#include <cstdint> // int64_t
#include <ctime>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include <QString>

#include <libtransmission/converters.h>
#include <libtransmission/variant.h>

#include "QtCompat.h"

class QByteArray;
class QDateTime;

class Speed;
class TorrentHash;
struct Peer;
struct TorrentFile;
struct TrackerStat;

namespace tr::serializer
{

TR_DECLARE_CONVERTER(QDateTime)
TR_DECLARE_CONVERTER(QString)
TR_DECLARE_CONVERTER(Speed)
TR_DECLARE_CONVERTER(TorrentHash)

} // namespace tr::serializer

namespace trqt::variant_helpers
{
template<typename T>
auto getValue(tr_variant const* variant)
    requires std::is_same_v<T, int>
{
    std::optional<T> ret;

    if (auto value = int64_t{}; tr_variantGetInt(variant, &value))
    {
        ret = value;
    }

    return ret;
}

template<typename T>
bool change(T& setme, T const& value)
{
    bool const changed = setme != value;

    if (changed)
    {
        setme = value;
    }

    return changed;
}

bool change(Peer& setme, tr_variant const* var);
bool change(TorrentFile& setme, tr_variant const* var);
bool change(TrackerStat& setme, tr_variant const* var);

template<typename T>
bool change(T& setme, tr_variant const* var)
{
    return var && tr::serializer::set(setme, *var);
}

template<typename T>
bool change(std::vector<T>& setme, tr_variant const* value)
{
    bool changed = false;

    auto const n = tr_variantListSize(value);
    if (setme.size() != n)
    {
        setme.resize(n);
        changed = true;
    }

    for (size_t i = 0; i < n; ++i)
    {
        changed = change(setme[i], tr_variantListChild(const_cast<tr_variant*>(value), i)) || changed;
    }

    return changed;
}

///

template<typename T>
std::optional<T> dictFind(tr_variant* dict, tr_quark key)
{
    if (dict)
    {
        if (auto const* const map = dict->get_if<tr_variant::Map>())
        {
            if (auto const iter = map->find(key); iter != map->end())
            {
                return tr::serializer::to_value<T>(iter->second);
            }
        }
    }

    return {};
}
} // namespace trqt::variant_helpers
