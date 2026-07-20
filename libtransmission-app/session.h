// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <sigslot/signal.hpp>

#include <woke/woke.hpp>

namespace tr::app
{

class Prefs;

class Session
{
public:
    explicit Session(Prefs& prefs);
    Session(Session&&) = delete;
    Session(Session const&) = delete;
    Session& operator=(Session&&) = delete;
    Session& operator=(Session const&) = delete;
    virtual ~Session() = default;

    [[nodiscard]] bool should_inhibit_sleep() const;

protected:
    void set_session_is_local(bool is_local);
    void set_has_busy_torrents(bool has_busy);

private:
    void update_sleep_inhibit();

    Prefs& prefs_;
    woke::SleepInhibitor sleep_inhibitor_;
    bool session_is_local_ = false;
    bool has_busy_torrents_ = false;
    sigslot::scoped_connection prefs_connection_;
};

} // namespace tr::app
