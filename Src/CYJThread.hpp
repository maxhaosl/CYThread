/*
 * CYJThread License
 * -----------
 *
 * CYJThread is licensed under the terms of the MIT license reproduced below.
 * This means that CYThread is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2025 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
 /*
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
  * VERSION:  1.0.0
  * PURPOSE:  A cross-platform efficient and stable thread pool library.
  * CREATION: 2025.04.07
  * LCHANGE:  2025.04.07
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */

#ifndef __CY_JTHREAD_HPP__
#define __CY_JTHREAD_HPP__

#include "CYThread/CYThreadDefine.hpp"

#include <thread>

#if defined(__has_include)
#  if __has_include(<stop_token>)
#    if defined(__APPLE__)
#      if (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 110000) || \
          (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 140000) || \
          (defined(__TV_OS_VERSION_MIN_REQUIRED) && __TV_OS_VERSION_MIN_REQUIRED >= 140000) || \
          (defined(__WATCH_OS_VERSION_MIN_REQUIRED) && __WATCH_OS_VERSION_MIN_REQUIRED >= 70000)
#        define CYTHREAD_HAS_STD_STOP_TOKEN 1
#      else
#        define CYTHREAD_HAS_STD_STOP_TOKEN 0
#      endif
#    else
#      define CYTHREAD_HAS_STD_STOP_TOKEN 1
#    endif
#  else
#    define CYTHREAD_HAS_STD_STOP_TOKEN 0
#  endif
#else
#  define CYTHREAD_HAS_STD_STOP_TOKEN 1
#endif

#if defined(__ANDROID__) && !defined(CYTHREAD_FORCE_STD_STOP_TOKEN)
#  undef CYTHREAD_HAS_STD_STOP_TOKEN
#  define CYTHREAD_HAS_STD_STOP_TOKEN 0
#endif

#if CYTHREAD_HAS_STD_STOP_TOKEN
#  include <stop_token>
#endif
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <system_error>
#include <atomic>
#include <memory>

CYTHRAD_NAMESPACE_BEGIN

namespace detail
{
#if CYTHREAD_HAS_STD_STOP_TOKEN
    using stop_token = std::stop_token;
    using stop_source = std::stop_source;
#else
    class stop_state
    {
    public:
        void request_stop() noexcept { m_flag.store(true, std::memory_order_release); }
        [[nodiscard]] bool stop_requested() const noexcept { return m_flag.load(std::memory_order_acquire); }
    private:
        std::atomic<bool> m_flag{ false };
    };

    class stop_token
    {
    public:
        stop_token() noexcept = default;
        explicit stop_token(std::shared_ptr<stop_state> state) noexcept : m_state(std::move(state)) {}
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return m_state ? m_state->stop_requested() : false;
        }
    private:
        std::shared_ptr<stop_state> m_state;
        friend class stop_source;
    };

    class stop_source
    {
    public:
        stop_source()
            : m_state(std::make_shared<stop_state>())
        {
        }

        stop_token get_token() const noexcept { return stop_token(m_state); }
        void request_stop() noexcept
        {
            if (m_state)
            {
                m_state->request_stop();
            }
        }
    private:
        std::shared_ptr<stop_state> m_state;
    };
#endif
} // namespace detail

using CYStopToken = detail::stop_token;
using CYStopSource = detail::stop_source;

#if 0 //defined(CYTHREAD_WIN_OS)
using CYJThread = std::jthread;
#else
// CYJThread: 一个与 std::jthread 语义非常接近的实现（C++20）
// 设计要点：
//  - 将传入的可调用与参数 decay 后移动到线程 lambda 中（避免生命周期问题）
//  - 如果可调用接受 std::stop_token，则自动注入 stop_token
//  - 正确实现移动构造/移动赋值（移动赋值会在必要时 request_stop + join）
//  - 提供常用 API：joinable/join/detach/request_stop/get_stop_token/get_stop_source/get_id/native_handle/swap
//  - 跨平台统一实现（不依赖平台宏）
// 注意：若你的平台已经有 std::jthread 并希望使用系统实现，请将 alias_section 处改为 using CYJThread = std::jthread;

class CYJThread
{
public:
    CYJThread() noexcept = default;

    /**
     * template ctor: start thread immediately.
     * @param f: the callable to be executed in the new thread.
     * @param args: the arguments to be passed to the callable.
     */
    template<
        typename Function, typename... Args,
        typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<std::remove_reference_t<Function>>, CYJThread>>
    >
    explicit CYJThread(Function&& f, Args&&... args)
    {
        Start(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /**
     * destructor: if joinable, request_stop then join (best-effort).
     */
    ~CYJThread()
    {
        if (joinable())
        {
            // best-effort stop + join; swallow exceptions
            try { m_stopSource.request_stop(); }
            catch (...) {}
            try { m_thread.join(); }
            catch (...) {}
        }
    }

    /**
     * non-copyable.
     */
    CYJThread(const CYJThread&) = delete;
    CYJThread& operator=(const CYJThread&) = delete;

    /**
     * move ctor.
     */
    CYJThread(CYJThread&& other) noexcept(
        std::is_nothrow_move_constructible_v<std::thread>&&
        std::is_nothrow_move_constructible_v<CYStopSource>)
        : m_thread(std::move(other.m_thread)),
        m_stopSource(std::move(other.m_stopSource))
    {
    }

    /**
     * move assign: if this is joinable, request_stop and join, then move.
     */
    CYJThread& operator=(CYJThread&& other) noexcept(
        std::is_nothrow_move_assignable_v<std::thread>&&
        std::is_nothrow_move_assignable_v<CYStopSource>)
    {
        if (this != &other)
        {
            if (joinable())
            {
                // mirror std::jthread behavior: request_stop then join
                m_stopSource.request_stop();
                m_thread.join();
            }
            m_thread = std::move(other.m_thread);
            m_stopSource = std::move(other.m_stopSource);
        }
        return *this;
    }

    /**
     * swap.
     */
    void swap(CYJThread& other) noexcept
    {
        using std::swap;
        swap(m_thread, other.m_thread);
        swap(m_stopSource, other.m_stopSource);
    }

    /**
     * queries / control.
     */
    bool joinable() const noexcept { return m_thread.joinable(); }
    void join() { m_thread.join(); }
    void detach() { m_thread.detach(); }

    void request_stop() noexcept { m_stopSource.request_stop(); }
    CYStopToken get_stop_token() const noexcept { return m_stopSource.get_token(); }
    CYStopSource get_stop_source() const noexcept { return m_stopSource; } // copyable handle

    std::thread::id get_id() const noexcept { return m_thread.get_id(); }
    std::thread::native_handle_type native_handle() { return m_thread.native_handle(); }

    // Additional helper: join with timeout-like behavior is not provided because std::thread lacks timed join.
    // If needed, user can implement condition variables + stop_token in the callable.

    // Allows constructing from an existing std::thread (consumes it) — optional convenience
    explicit CYJThread(std::thread t)
        : m_thread(std::move(t)), m_stopSource()
    {
        // Note: stop_source is default constructed. The user may want to manage stop separately.
    }

private:
    /**
     * core start routine.
     * @param f: the callable to be executed in the new thread.
     * @param args: the arguments to be passed to the callable.
     */
    template<typename Function, typename... Args>
    void Start(Function&& f, Args&&... args)
    {
        // Make decayed copies of callable and arguments
        using FnDecay = std::decay_t<Function>;
        FnDecay fn(std::forward<Function>(f));

        auto boundArgs = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

        // initialize stop_source for this thread
        m_stopSource = CYStopSource{};
        CYStopToken token = m_stopSource.get_token();

        // Move into thread: fn and boundArgs captured by move
        m_thread = std::thread(
            [fn = std::move(fn), token, boundArgs = std::move(boundArgs)]() mutable
            {
                // expand tuple and call
                InvokeWithToken(fn, token, boundArgs,
                    std::make_index_sequence<std::tuple_size_v<decltype(boundArgs)>>{});
            });
    }

    /**
     * helper function to call fn with token and args.
     * @param fn: the callable to be executed in the new thread.
     * @param token: the stop_token to be passed to the callable.
     * @param tup: the tuple of arguments to be passed to the callable.
     * @param Indices: the indices of the arguments to be passed to the callable.
     */
    template<typename Function, typename Tuple, std::size_t... Indices>
    static void InvokeWithToken(Function& fn, CYStopToken token, Tuple& tup, std::index_sequence<Indices...>)
    {
        Call(fn, token, std::get<Indices>(tup)...);
    }

    /**
     * helper function to call fn with args.
     * @param fn: the callable to be executed in the new thread.
     * @param args: the arguments to be passed to the callable.
     */
    template<typename Function, typename... Args>
    static void Call(Function& fn, CYStopToken token, Args&&... args)
    {
        if constexpr (std::is_invocable_v<Function, CYStopToken, Args...>)
        {
            std::invoke(fn, token, std::forward<Args>(args)...);
        }
        else
        {
            std::invoke(fn, std::forward<Args>(args)...);
        }
    }

private:
    std::thread m_thread;
    CYStopSource m_stopSource;
};

// If you prefer to use system std::jthread when available, uncomment the alias below
// (and remove or guard the above implementation as needed).
// using CYJThread = std::jthread;

#endif // defined(CYTHREAD_WIN_OS)

CYTHRAD_NAMESPACE_END

#endif // __CY_JTHREAD_HPP__