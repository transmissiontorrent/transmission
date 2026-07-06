// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <type_traits>
#include <utility>

#include "libtransmission-app/rpc-client.h"

namespace tr::app
{

// Executes RPC steps in order.
//
// A step is one of:
//   * request, with prior response: [](RpcResponse const&, RpcClient::ResponseFunc done){ rpc.exec(..., done); }
//   * request, first step:          [](RpcClient::ResponseFunc done){ rpc.exec(..., done); }
//   * auxiliary, with response:     [](RpcResponse const& r){ use(r); }
//   * auxiliary, no argument:       []{ ... }
//
// Typical use: make() -> add() -> (optional) finally() -> run().
// You don't need to keep a handle to the queue after calling run().
class RpcQueue : public std::enable_shared_from_this<RpcQueue>
{
public:
    [[nodiscard]] static std::shared_ptr<RpcQueue> make()
    {
        return std::make_shared<RpcQueue>(PrivateTag{});
    }

    // enable_shared_from_this requires a public-ish ctor for make_shared;
    // the PrivateTag keeps callers on make() so instances are always shared.
    struct PrivateTag {
        explicit PrivateTag() = default;
    };

    explicit RpcQueue(PrivateTag /*unused*/)
    {
    }

    RpcQueue(RpcQueue&&) = delete;
    RpcQueue(RpcQueue const&) = delete;
    RpcQueue& operator=(RpcQueue&&) = delete;
    RpcQueue& operator=(RpcQueue const&) = delete;

    template<typename Func>
    void add(Func&& func)
    {
        queue_.emplace(normalize_func(std::forward<Func>(func)), ErrorHandlerFunction{});
    }

    template<typename Func, typename ErrorHandler>
    void add(Func&& func, ErrorHandler&& error_handler)
    {
        queue_.emplace(
            normalize_func(std::forward<Func>(func)),
            normalize_error_handler(std::forward<ErrorHandler>(error_handler)));
    }

    // `func` runs once when the chain ends, whether or not the chain succeeded
    template<typename Func>
    void finally(Func&& func)
    {
        finally_ = make_copyable(std::forward<Func>(func));
    }

    // The first step is run synchronously (so it may capture locals by reference).
    void run()
    {
        self_ = shared_from_this();
        run_next(RpcResponse{});
    }

private:
    using Continue = RpcClient::ResponseFunc; // std::function<void(RpcResponse)>

    // Internally queued step: takes the previous response and a continuation
    // that must be invoked (once) with this step's result.
    using QueuedFunction = std::function<void(RpcResponse const&, Continue)>;

    // Internally stored error handler: takes the failing response, returns nothing.
    using ErrorHandlerFunction = std::function<void(RpcResponse const&)>;

    [[nodiscard]] static RpcResponse make_ok_response()
    {
        auto ret = RpcResponse{};
        ret.success = true;
        return ret;
    }

    void run_next(RpcResponse const& prev)
    {
        auto [func, error_handler] = std::move(queue_.front());
        queue_.pop();

        next_error_handler_ = std::move(error_handler);

        auto self = self_; // keep alive across the (possibly async) step
        func(prev, [self](RpcResponse result) { self->step_finished(std::move(result)); });
    }

    void step_finished(RpcResponse result)
    {
        // we can't handle network errors: abort the queue and pass the error upward.
        if (result.network_error) {
            finish();
            return;
        }

        // call the user error handler for ordinary (non-network) errors.
        if (!result.success && next_error_handler_) {
            next_error_handler_(result);
        }

        // run the next step if there is one and we succeeded.
        if (result.success && !std::empty(queue_)) {
            run_next(result);
            return;
        }

        finish();
    }

    void finish()
    {
        if (finally_) {
            finally_();
        }
        self_.reset();
    }

    template<typename Func>
    static auto make_copyable(Func&& func)
    {
        using FuncType = std::remove_cvref_t<Func>;
        auto shared_func = std::make_shared<FuncType>(std::forward<Func>(func));
        return [shared_func](auto&&... args) -> decltype(auto) {
            return std::invoke(*shared_func, std::forward<decltype(args)>(args)...);
        };
    }

    // request step, takes previous response and a continuation
    template<typename Func>
        requires std::invocable<Func, RpcResponse const&, Continue>
    static QueuedFunction normalize_func(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& prev, Continue done) mutable {
            std::invoke(func, prev, std::move(done));
        };
    }

    // first request step, takes only a continuation (ignores previous response)
    template<typename Func>
        requires std::invocable<Func, Continue> && (!std::invocable<Func, RpcResponse const&, Continue>)
    static QueuedFunction normalize_func(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& /*prev*/, Continue done) mutable {
            std::invoke(func, std::move(done));
        };
    }

    // auxiliary step, takes the previous response and returns nothing
    template<typename Func>
        requires std::invocable<Func, RpcResponse const&> && (!std::invocable<Func, Continue>) &&
        (!std::invocable<Func, RpcResponse const&, Continue>)
    static QueuedFunction normalize_func(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& prev, Continue done) mutable {
            std::invoke(func, prev);
            done(make_ok_response());
        };
    }

    // auxiliary step, takes nothing and returns nothing
    template<typename Func>
        requires std::invocable<Func> && (!std::invocable<Func, RpcResponse const&>) && (!std::invocable<Func, Continue>)
    static QueuedFunction normalize_func(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& /*prev*/, Continue done) mutable {
            std::invoke(func);
            done(make_ok_response());
        };
    }

    // error handler taking the failing response
    template<typename Func>
        requires std::invocable<Func, RpcResponse const&>
    static ErrorHandlerFunction normalize_error_handler(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& r) mutable {
            std::invoke(func, r);
        };
    }

    // error handler taking nothing
    template<typename Func>
        requires std::invocable<Func> && (!std::invocable<Func, RpcResponse const&>)
    static ErrorHandlerFunction normalize_error_handler(Func&& func)
    {
        auto copyable_func = make_copyable(std::forward<Func>(func));
        return [func = std::move(copyable_func)](RpcResponse const& /*r*/) mutable {
            std::invoke(func);
        };
    }

    std::shared_ptr<RpcQueue> self_;
    std::queue<std::pair<QueuedFunction, ErrorHandlerFunction>> queue_;
    ErrorHandlerFunction next_error_handler_;
    std::function<void()> finally_;
};

} // namespace tr::app
