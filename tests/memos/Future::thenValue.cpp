template <typename...>
struct ArgType;

template <typename Arg, typename... Args>
struct ArgType<Arg, Args...> {
  typedef Arg FirstArg;
  typedef ArgType<Args...> Tail;
};

template <>
struct ArgType<> {
  typedef void FirstArg;
};

template <std::size_t I>
using index_constant = std::integral_constant<std::size_t, I>;

template <bool isTry_, typename F, typename... Args>
struct argResult {
  using Function = F;
  using ArgList = ArgType<Args...>;
  using Result = invoke_result_t<F, Args...>;
  using ArgsSize = index_constant<sizeof...(Args)>;
  static constexpr bool isTry() { return isTry_; }
};

template <typename T>
struct isFutureOrSemiFuture : std::false_type {
  using Inner = lift_unit_t<T>;
  using Return = Inner;
};

template <typename T>
struct isFutureOrSemiFuture<Try<T>> : std::false_type {
  using Inner = lift_unit_t<T>;
  using Return = Inner;
};

template <typename T, typename F>
struct valueCallableResult {
  // T&&: forward reference
  typedef detail::argResult<false, F, T&&> Arg;
  typedef isFutureOrSemiFuture<typename Arg::Result> ReturnsFuture;
  typedef typename ReturnsFuture::Inner value_type;
  typedef typename Arg::ArgList::FirstArg FirstArg;
  typedef Future<value_type> Return;
};

template <typename T, typename F>
auto wrapInvoke(folly::Try<T>&& t, F&& f) {
  auto fn = [&]() {
    return static_cast<F&&>(f)(
        t.template get<
            false,
            typename futures::detail::valueCallableResult<T, F>::FirstArg>());
  };
  using FnResult = decltype(fn());
  using Wrapper = InvokeResultWrapper<FnResult>;
  if (t.hasException()) {
    return Wrapper::wrapException(std::move(t).exception());
  }
  return Wrapper::wrapResult(fn);
}

// 注意, 下面的Try<T>&&是右值引用, 不是万能引用 (要求T&&). 另外, Executor*
// 可以通过implicit的方式转成Executor::KeepAlive<>.
template <
    typename T,
    typename F,
    typename = std::enable_if_t<is_invocable_v<F, Executor*, Try<T>&&>>>
struct tryExecutorCallableResult {
  typedef detail::argResult<true, F, Executor::KeepAlive<>&&, Try<T>&&> Arg;
  // ReturnsFuture::value为true, 表示F返回的结果是future类型
  typedef isFutureOrSemiFuture<typename Arg::Result> ReturnsFuture;
  typedef typename ReturnsFuture::Inner value_type;
  typedef Future<value_type> Return;
};

template <class T>
template <typename F>
Future<typename futures::detail::valueCallableResult<T, F>::value_type>
Future<T>::thenValue(F&& func) && {
  // static_cast<F&&>(func): 即std::forward<F>(func), 转发引用类型
  auto lambdaFunc = [f = static_cast<F&&>(func)](
                        Executor::KeepAlive<>&&, folly::Try<T>&& t) mutable {
    // 将结果封装为Try(...)
    return futures::detail::wrapInvoke(std::move(t), static_cast<F&&>(f));
  };
  using R = futures::detail::tryExecutorCallableResult<T, decltype(lambdaFunc)>;
  return this->thenImplementation(
      std::move(lambdaFunc), R{}, futures::detail::InlineContinuation::forbid);
}