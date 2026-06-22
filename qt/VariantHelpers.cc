// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#include "VariantHelpers.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include <QDateTime>
#include <QUrl>

#include <libtransmission/converters.h>
#include <libtransmission/web-utils.h>

#include "Application.h" // qApp
#include "QtCompat.h"
#include "Speed.h"
#include "Torrent.h"

namespace ser = tr::serializer;

namespace trqt::variant_helpers
{

bool change(double& setme, double const& value)
{
    bool const changed = std::fabs(setme - value) > std::numeric_limits<double>::epsilon();

    if (changed)
    {
        setme = value;
    }

    return changed;
}

bool change(Speed& setme, tr_variant const* value)
{
    auto const byps = getValue<int>(value);
    return byps && change(setme, Speed{ *byps, Speed::Units::Byps });
}

bool change(TorrentHash& setme, tr_variant const* value)
{
    auto const hash_string = getValue<std::string_view>(value);
    return hash_string && change(setme, TorrentHash{ *hash_string });
}

bool change(Peer& setme, tr_variant const* value)
{
    return !ser::load(setme, Peer::Fields, *value).empty();
}

bool change(TorrentFile& setme, tr_variant const* value)
{
    return !ser::load(setme, TorrentFile::Fields, *value).empty();
}

bool change(TrackerStat& setme, tr_variant const* value)
{
    auto const changed_keys = ser::load(setme, TrackerStat::Fields, *value);
    auto const site_changed = std::ranges::binary_search(changed_keys, TR_KEY_announce) ||
        std::ranges::binary_search(changed_keys, TR_KEY_sitename);

    if (site_changed && !setme.announce.isEmpty() && trApp != nullptr)
    {
        if (setme.sitename.isEmpty())
        {
            auto const announce_str = setme.announce.toStdString();
            if (auto const parsed = tr_urlParse(announce_str))
            {
                auto const sitename = parsed->sitename;
                setme.sitename = QString::fromUtf8(
                    std::data(sitename),
                    static_cast<IF_QT6(qsizetype, int)>(std::size(sitename)));
            }
        }

        setme.announce = trApp->intern(setme.announce);
        trApp->load_favicon(setme.announce);
    }

    return !changed_keys.empty();
}

///

namespace
{
bool toInt(tr_variant const& src, int* tgt)
{
    if (auto const val = src.value_if<int64_t>())
    {
        if (*val < std::numeric_limits<int>::min() || *val > std::numeric_limits<int>::max())
        {
            return false;
        }

        *tgt = static_cast<int>(*val);
        return true;
    }

    return false;
}

tr_variant fromInt(int const& val)
{
    return static_cast<int64_t>(val);
}

// ---

bool toQDateTime(tr_variant const& src, QDateTime* tgt)
{
    if (auto const val = ser::to_value<int64_t>(src))
    {
        *tgt = QDateTime::fromSecsSinceEpoch(*val);
        return true;
    }

    return false;
}

tr_variant fromQDateTime(QDateTime const& src)
{
    return ser::to_variant(int64_t{ src.toSecsSinceEpoch() });
}

// ---

bool toQString(tr_variant const& src, QString* tgt)
{
    if (auto const val = src.value_if<std::string_view>())
    {
        *tgt = QString::fromUtf8(std::data(*val), static_cast<IF_QT6(qsizetype, int)>(std::size(*val)));
        return true;
    }

    return false;
}

tr_variant fromQString(QString const& val)
{
    return val.toStdString();
}

// ---

bool toSpeed(tr_variant const& src, Speed* tgt)
{
    if (auto const val = ser::to_value<int64_t>(src))
    {
        *tgt = Speed{ *val, Speed::Units::Byps };
        return true;
    }

    return false;
}

tr_variant fromSpeed(Speed const& src)
{
    return ser::to_variant(static_cast<int64_t>(src.base_quantity()));
}
} // namespace

} // namespace trqt::variant_helpers

// ---
// `Converter<T>` out-of-line definitions for Qt-side types. Each forwards
// to the matching helper in the unnamed namespace above.

namespace tr::serializer
{
namespace vh = trqt::variant_helpers;

tr_variant Converter<int>::to_variant(int const& src)
{
    return vh::fromInt(src);
}
bool Converter<int>::to_value(tr_variant const& src, int* tgt)
{
    return vh::toInt(src, tgt);
}

tr_variant Converter<QDateTime>::to_variant(QDateTime const& src)
{
    return vh::fromQDateTime(src);
}
bool Converter<QDateTime>::to_value(tr_variant const& src, QDateTime* tgt)
{
    return vh::toQDateTime(src, tgt);
}

tr_variant Converter<QString>::to_variant(QString const& src)
{
    return vh::fromQString(src);
}
bool Converter<QString>::to_value(tr_variant const& src, QString* tgt)
{
    return vh::toQString(src, tgt);
}

tr_variant Converter<Speed>::to_variant(Speed const& src)
{
    return vh::fromSpeed(src);
}
bool Converter<Speed>::to_value(tr_variant const& src, Speed* tgt)
{
    return vh::toSpeed(src, tgt);
}

} // namespace tr::serializer
