#pragma once

#include <type_traits>

#include "type_list.h"

namespace t9 {

/// @cond private
namespace detail {

template <typename R, typename... A>
struct func_type_def {
  using result_type = R;
  using args_type = type_list<A...>;
};

template <typename T>
struct func_traits_impl;

template <typename R, typename... A>
struct func_traits_impl<R (*)(A...)> : func_type_def<R, A...> {};

template <typename R, class T, typename... A>
struct func_traits_impl<R (T::*)(A...)> : func_type_def<R, A...> {};

template <typename R, class T, typename... A>
struct func_traits_impl<R (T::*)(A...) const> : func_type_def<R, A...> {};

template <typename F, typename = void>
struct func_traits : func_traits_impl<F> {};

// @see http://stackoverflow.com/a/15394527
template <typename T>
struct func_traits<T, decltype(void(&T::operator()))>
    : func_traits_impl<decltype(&T::operator())> {};

}  // namespace detail
/// @endcond

template <typename F>
struct func_traits : detail::func_traits<std::decay_t<F>> {};

template <typename F>
using result_type_t = typename func_traits<F>::result_type;

template <typename F>
using args_type_t = typename func_traits<F>::args_type;

}  // namespace t9
