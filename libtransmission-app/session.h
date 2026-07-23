// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <cstdint>
#include <optional>

#include <sigslot/signal.hpp>

#include <woke/woke.hpp>

namespace tr::app
{

class Prefs;

class Session
{
public:
    // How the app reaches its `tr_session`:
    enum class Type : uint8_t {
        InProcess, // the session runs inside this process
        Local, // a separate daemon process on this machine
        Remote, // a daemon on another machine
    };

    explicit Session(Prefs& prefs);
    Session(Session&&) = delete;
    Session(Session const&) = delete;
    Session& operator=(Session&&) = delete;
    Session& operator=(Session const&) = delete;
    virtual ~Session() = default;

    [[nodiscard]] constexpr std::optional<Type> type() const noexcept
    {
        return session_type_;
    }

    // keep the machine awake: a local-filesystem session with active transfers
    [[nodiscard]] bool should_inhibit_sleep() const;

    // keep this process un-throttled: only when it hosts the session itself
    [[nodiscard]] bool should_inhibit_nap() const noexcept
    {
        return session_type_ == Type::InProcess;
    }

protected:
    void set_session_type(std::optional<Type> type);
    void set_has_busy_torrents(bool has_busy);

private:
    void update_sleep_inhibit();
    void update_nap_inhibit();

    [[nodiscard]] bool is_local_filesystem() const noexcept
    {
        return session_type_.value_or(Type::Remote) != Type::Remote;
    }

    Prefs& prefs_;
    woke::SleepInhibitor sleep_inhibitor_;
    woke::NapInhibitor nap_inhibitor_;
    std::optional<Type> session_type_;
    bool has_busy_torrents_ = false;
    sigslot::scoped_connection prefs_connection_;
};

} // namespace tr::app
