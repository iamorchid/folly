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
  using ArgList = ArgType<Args...>;
  using Result = invoke_result_t<F, Args...>;
  using ArgsSize = index_constant<sizeof...(Args)>;

  // isTry()表示F是否接受Try类型的参数
  static constexpr bool isTry() { return isTry_; }
};

template <typename T>
struct isFutureOrSemiFuture : std::false_type {
  using Inner = lift_unit_t<T>;
};

template <typename T>
struct isFutureOrSemiFuture<Try<T>> : std::false_type {
  using Inner = lift_unit_t<T>;
};

template <typename T>
struct isFutureOrSemiFuture<Future<T>> : std::true_type {
  typedef T Inner;
};

template <typename T>
struct isFutureOrSemiFuture<Future<Try<T>>> : std::true_type {
  typedef T Inner;
};

template <typename T>
struct isFutureOrSemiFuture<SemiFuture<T>> : std::true_type {
  typedef T Inner;
};

template <typename T>
struct isFutureOrSemiFuture<SemiFuture<Try<T>>> : std::true_type {
  typedef T Inner;
};

template <typename T, typename F>
struct tryCallableResult {
  // 下面的true表示F的接受的参数类型为Try类型
  typedef detail::argResult<true, F, Try<T>&&> Arg;
  typedef isFutureOrSemiFuture<typename Arg::Result> ReturnsFuture;
  typedef typename ReturnsFuture::Inner value_type;
};

template <typename T, typename F>
struct valueCallableResult {
  // T&&: forward reference
  typedef detail::argResult<false, F, T&&> Arg;
  typedef isFutureOrSemiFuture<typename Arg::Result> ReturnsFuture;
  typedef typename ReturnsFuture::Inner value_type;
};

template <class T>
class Try {
  template <bool isTry, typename R>
  typename std::enable_if<isTry, R>::type get() {
    return std::forward<R>(*this);
  }

  template <bool isTry, typename R>
  typename std::enable_if<!isTry, R>::type get() {
    return std::forward<R>(value());
  }
};

namespace detail {

// InvokeResultWrapper and wrapInvoke enable wrapping a result value in its
// nearest Future-type counterpart capable of also carrying an exception.
// e.g.
//    (semi)Future<T> -> (semi)Future<T>           (no change)
//    Try<T>          -> Try<T>                    (no change)
//    void            -> Try<folly::Unit>
//    T               -> Try<T>
template <typename T>
struct InvokeResultWrapperBase {
  template <typename F>
  static T wrapResult(F fn) {
    return T(fn());
  }
  static T wrapException(exception_wrapper&& e) { return T(std::move(e)); }
};
template <typename T>
struct InvokeResultWrapper : InvokeResultWrapperBase<Try<T>> {};
template <typename T>
struct InvokeResultWrapper<Try<T>> : InvokeResultWrapperBase<Try<T>> {};
template <typename T>
struct InvokeResultWrapper<SemiFuture<T>>
    : InvokeResultWrapperBase<SemiFuture<T>> {};
template <typename T>
struct InvokeResultWrapper<Future<T>> : InvokeResultWrapperBase<Future<T>> {};
template <>
struct InvokeResultWrapper<void> : InvokeResultWrapperBase<Try<Unit>> {
  template <typename F>
  static Try<Unit> wrapResult(F fn) {
    fn();
    return Try<Unit>(unit);
  }
};

template <typename T, typename F>
auto wrapInvoke(folly::Try<T>&& t, F&& f) {
  auto fn = [&]() { return static_cast<F&&>(f)(t.template get<false, T&&>()); };
  using FnResult = decltype(fn());
  using Wrapper = InvokeResultWrapper<FnResult>;
  if (t.hasException()) {
    return Wrapper::wrapException(std::move(t).exception());
  }
  return Wrapper::wrapResult(fn);
}

}

template <typename T, typename F>
struct tryExecutorCallableResult {
  // 注意, 下面的Try<T>&&是右值引用, 不是万能引用 (要求T&&). 另外, Executor*
  // 可以通过implicit的方式转成Executor::KeepAlive<>.
  typedef detail::argResult<true, F, Executor::KeepAlive<>&&, Try<T>&&> Arg;
  typedef isFutureOrSemiFuture<typename Arg::Result> ReturnsFuture;
  typedef typename ReturnsFuture::Inner value_type;
};

namespace detail {
template <typename T, typename F>
class CoreCallbackState {
  using DF = folly::decay_t<F>;

 public:
  CoreCallbackState(Promise<T>&& promise, F&& func) noexcept(
      noexcept(DF(static_cast<F&&>(func))))
      : func_(static_cast<F&&>(func)),
        core_(std::exchange(promise.core_, nullptr)) {
    assert(before_barrier());
  }

  template <typename... Args>
  auto invoke(Args&&... args) noexcept(
      noexcept(FOLLY_DECLVAL(F&&)(static_cast<Args&&>(args)...))) {
    assert(before_barrier());
    return static_cast<F&&>(func_)(static_cast<Args&&>(args)...);
  }

  template <typename... Args>
  auto tryInvoke(Args&&... args) noexcept {
    return makeTryWith([&] { return invoke(static_cast<Args&&>(args)...); });
  }

  void setTry(Executor::KeepAlive<>&& keepAlive, Try<T>&& t) {
    stealPromise().setTry(std::move(keepAlive), std::move(t));
  }

  void setException(Executor::KeepAlive<>&& keepAlive, exception_wrapper&& ew) {
    setTry(std::move(keepAlive), Try<T>(std::move(ew)));
  }

  Promise<T> stealPromise() noexcept {
    assert(before_barrier());
    func_.~DF();
    return Promise<T>{
        MakeRetrievedFromStolenCoreTag{}, *std::exchange(core_, nullptr)};
  }
};
// f对应的是函数之外的其他callable, 比如函数对象、lambda等. 这种情况下, 
// 初始化CoreCallbackState可能会抛出异常.
template <typename T, typename F>
auto makeCoreCallbackState(Promise<T>&& p, F&& f) noexcept(
    noexcept(CoreCallbackState<T, F>(std::move(p), static_cast<F&&>(f)))) {
  return CoreCallbackState<T, F>(std::move(p), static_cast<F&&>(f));
}

// f对应的函数
template <typename T, typename R, typename... Args>
auto makeCoreCallbackState(Promise<T>&& p, R (&f)(Args...)) noexcept {
  return CoreCallbackState<T, R (*)(Args...)>(std::move(p), &f);
}

}

namespace folly /* Try-inl.h */ {

template <typename T>
struct isTry : std::false_type {};

template <typename T>
struct isTry<Try<T>> : std::true_type {};


template <typename F>
typename std::enable_if<
    !std::is_same<invoke_result_t<F>, void>::value,
    Try<invoke_result_t<F>>>::type
makeTryWithNoUnwrap(F&& f) noexcept {
  using ResultType = invoke_result_t<F>;
  try {
    return Try<ResultType>(f());
  } catch (...) {
    return Try<ResultType>(exception_wrapper(current_exception()));
  }
}

template <typename F>
typename std::
    enable_if<std::is_same<invoke_result_t<F>, void>::value, Try<void>>::type
    makeTryWithNoUnwrap(F&& f) noexcept {
  try {
    f();
    return Try<void>();
  } catch (...) {
    return Try<void>(exception_wrapper(current_exception()));
  }
}

template <typename F>
typename std::
    enable_if<!isTry<invoke_result_t<F>>::value, Try<invoke_result_t<F>>>::type
    makeTryWith(F&& f) noexcept {
  return makeTryWithNoUnwrap(std::forward<F>(f));
}

template <typename F>
typename std::enable_if<isTry<invoke_result_t<F>>::value, invoke_result_t<F>>::
    type
    makeTryWith(F&& f) noexcept {
  using ResultType = invoke_result_t<F>;
  try {
    return f();
  } catch (...) {
    return ResultType(exception_wrapper(current_exception()));
  }
}

}

// MSVC 2017 Update 7 released with a bug that causes issues expanding to an
// empty parameter pack when invoking a templated member function. It should
// be fixed for MSVC 2017 Update 8.
// TODO: Remove.
namespace detail_msvc_15_7_workaround {
template <typename R, std::size_t S>
using IfArgsSizeIs = std::enable_if_t<R::Arg::ArgsSize::value == S, int>;
template <typename R, typename State, typename T, IfArgsSizeIs<R, 0> = 0>
decltype(auto) invoke(
    R, State& state, Executor::KeepAlive<>&&, Try<T>&& /* t */) {
  return state.invoke();
}
template <typename R, typename State, typename T, IfArgsSizeIs<R, 2> = 0>
decltype(auto) invoke(R, State& state, Executor::KeepAlive<>&& ka, Try<T>&& t) {
  using Arg1 = typename R::Arg::ArgList::Tail::FirstArg;
  return state.invoke(
      std::move(ka), std::move(t).template get<R::Arg::isTry(), Arg1>());
}
template <typename R, typename State, typename T, IfArgsSizeIs<R, 0> = 0>
decltype(auto) tryInvoke(
    R, State& state, Executor::KeepAlive<>&&, Try<T>&& /* t */) {
  return state.tryInvoke();
}
template <typename R, typename State, typename T, IfArgsSizeIs<R, 2> = 0>
decltype(auto) tryInvoke(
    R, State& state, Executor::KeepAlive<>&& ka, Try<T>&& t) {
  using Arg1 = typename R::Arg::ArgList::Tail::FirstArg;
  return state.tryInvoke(
      std::move(ka), std::move(t).template get<R::Arg::isTry(), Arg1>());
}
} // namespace detail_msvc_15_7_workaround

// Variant: returns a value
// e.g. f.then([](Try<T>&& t){ return t.value(); });
template <class T>
template <typename F, typename R>
typename std::enable_if< //
    !R::ReturnsFuture::value,
    Future<typename R::value_type>>::type
FutureBase<T>::thenImplementation(
    F&& func, R, futures::detail::InlineContinuation allowInline) {
  static_assert(R::Arg::ArgsSize::value == 2, "Then must take two arguments");
  using B = typename R::ReturnsFuture::Inner;

  // 每个promise都有自己的core_实例
  auto fp = FutureBaseHelper::makePromiseContractForThen<B>(
      this->getCore() /* 从已有core实例复制一些参数 */, this->getExecutor());

  // 当前future有结果后, 会调用这里的callback, 将结果传给func。之后再将func的结果
  // 作用于新建的promise变量p, 进而传导给和p关联的Core对象(和f关联的是同一个Core).
  this->setCallback_(
      [state = futures::detail::makeCoreCallbackState(
           std::move(fp.promise), static_cast<F&&>(func))](
          Executor::KeepAlive<>&& ka, Try<T>&& t) mutable {
        // 目前看起来, R固定为tryExecutorCallableResult, 即R::Arg::isTry()总为true
        if (!R::Arg::isTry() && t.hasException()) {
          // 如果func不接受Try<T>作为参数, 则它无法处理Try<T>中的异常. 此时, 会跳过
          // func的执行, 直接将异常设置到fp关联的core实例上.
          state.setException(std::move(ka), std::move(t.exception()));
        } else {
          auto propagateKA = ka.copy();
          // state.setTry会执行内部promise的setTry方法, 并传递给关联的core_实例
          state.setTry(std::move(propagateKA), makeTryWith([&] {
                         // 这里会执行CoreCallbackState的invoke或者tryInvoke方法, 
                         // 它们会进一步执行到func并发返回func的结果
                         return detail_msvc_15_7_workaround::invoke(
                             R{}, state, std::move(ka), std::move(t));
                       }));
        }
      },
      allowInline);

  // 上面的callback执行后, 结果不会主动返回到Future身上, 需要调用Future::value()
  // 等函数从关联的Core对象上去获取. 或者通过f.thenValue(...)往f关联的core上注册
  // callback, 当f对应的promise fullfilled时候, 会触发这个callback.
  return std::move(fp.future);
}

// Variant: returns a Future
// e.g. f.then([](T&& t){ return makeFuture<T>(t); });
template <class T>
template <typename F, typename R>
typename std::enable_if< //
    R::ReturnsFuture::value,
    Future<typename R::value_type>>::type
FutureBase<T>::thenImplementation(
    F&& func, R, futures::detail::InlineContinuation allowInline) {
  static_assert(R::Arg::ArgsSize::value == 2, "Then must take two arguments");
  using B = typename R::ReturnsFuture::Inner;
  auto fp = FutureBaseHelper::makePromiseContractForThen<B>(
      this->getCore(), this->getExecutor());
  this->setCallback_(
      [state = futures::detail::makeCoreCallbackState(
           std::move(fp.promise), static_cast<F&&>(func))](
          Executor::KeepAlive<>&& ka, Try<T>&& t) mutable {
        if (!R::Arg::isTry() && t.hasException()) {
          state.setException(std::move(ka), std::move(t.exception()));
        } else {
          // Ensure that if function returned a SemiFuture we correctly chain
          // potential deferral.
          // tf2类型为: Try<Future<T>> 或者 Try<SemiFuture<T>>
          auto tf2 = detail_msvc_15_7_workaround::tryInvoke(
              R{}, state, ka.copy(), std::move(t));
          if (tf2.hasException()) {
            state.setException(std::move(ka), std::move(tf2.exception()));
          } else {
            auto statePromise = state.stealPromise();
            auto tf3 /* Future<T> */ = chainExecutor(std::move(ka), *std::move(tf2));
            std::exchange(statePromise.core_, nullptr)
                ->setProxy(std::exchange(tf3.core_, nullptr));
          }
        }
      },
      allowInline);

  return std::move(fp.future);
}

// 注意和Future<T>::thenTry的区别
template <class T>
template <typename F>
Future<typename futures::detail::valueCallableResult<T, F>::value_type>
Future<T>::thenValue(F&& func) && {
  // static_cast<F&&>(func): 即std::forward<F>(func), 转发引用类型
  auto lambdaFunc = [f = static_cast<F&&>(func)](
                        Executor::KeepAlive<>&&, folly::Try<T>&& t) mutable {
    // 将结果封装为Try(Y), Y为F的返回值 (如果f的返回值是Try或者Future, 则原样返回)
    return futures::detail::wrapInvoke(std::move(t), static_cast<F&&>(f));
  };

  // W: wrapper, 即上面的lambda是对func的封装 (参数和返回值的封装)
  using W = decltype(lambdaFunc);

  using R = futures::detail::tryExecutorCallableResult<T, W>;
  auto policy = futures::detail::InlineContinuation::forbid;
  return this->thenImplementation(static_cast<W&&>(lambdaFunc), R{}, policy);
}

template <class T>
template <typename F>
Future<typename futures::detail::tryCallableResult<T, F>::value_type>
Future<T>::thenTry(F&& func) && {
  auto lambdaFunc = [f = static_cast<F&&>(func)](
                        folly::Executor::KeepAlive<>&&,
                        folly::Try<T>&& t) mutable {
    // 函数func接受Try<T>作为参数, 返回值并不要求为Try类型. thenImplementation实现中, 
    // 将结果传给新建的promise时, 会统一通过makeTryWith将返回值转成Try类型.
    return static_cast<F&&>(f)(std::move(t));
  };
  using W = decltype(lambdaFunc);
  using R = futures::detail::tryExecutorCallableResult<T, W>;
  auto policy = futures::detail::InlineContinuation::forbid;
  return this->thenImplementation(static_cast<W&&>(lambdaFunc), R{}, policy);
}