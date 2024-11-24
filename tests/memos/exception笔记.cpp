template <typename...>
struct Conjunction : std::true_type {};

template <typename T>
struct Conjunction<T> : T {};

// 这里的T都是继承std::true_type或者std::false_type
template <typename T, typename... TList>
struct Conjunction<T, TList...>
    : std::conditional<T::value, Conjunction<TList...>, T>::type {};


template <class Ex>
using IsStdException = std::is_base_of<std::exception, std::decay_t<Ex>>;

template <typename T>
struct Negation : std::bool_constant<!T::value> {};

template <bool... Bs>
struct Bools {
  using valid_type = bool;
  static constexpr std::size_t size() { return sizeof...(Bs); }
};

//  Lighter-weight than Conjunction, but evaluates all sub-conditions eagerly.
template <class... Ts>
struct StrictConjunction
    : std::is_same<Bools<Ts::value...>, Bools<(Ts::value || true)...>> {};

template <class T>
struct IsRegularExceptionType
    : StrictConjunction<
          std::is_copy_constructible<T>,
          Negation<std::is_base_of<exception_wrapper, T>>,
          Negation<std::is_abstract<T>>> {};

#define FOLLY_REQUIRES_DEF(...) \
  std::enable_if_t<static_cast<bool>(__VA_ARGS__), long>

#define FOLLY_REQUIRES(...) FOLLY_REQUIRES_DEF(__VA_ARGS__) = __LINE__


class exception_wrapper final {

  //! \pre `typeid(ex) == typeid(typename decay<Ex>::type)`
  //! \post `bool(*this)`
  //! \post `type() == &typeid(ex)`
  //! \note Exceptions of types derived from `std::exception` can be implicitly
  //!     converted to an `exception_wrapper`.
  template <
      class Ex,
      class Ex_ = std::decay_t<Ex>,
      FOLLY_REQUIRES(
          Conjunction<IsStdException<Ex_>, IsRegularExceptionType<Ex_>>::value)>
  /* implicit */ exception_wrapper(Ex&& ex);

  //! \pre `typeid(ex) == typeid(typename decay<Ex>::type)`
  //! \post `bool(*this)`
  //! \post `type() == &typeid(ex)`
  //! \note Exceptions of types not derived from `std::exception` can still be
  //!     used to construct an `exception_wrapper`, but you must specify
  //!     `std::in_place` as the first parameter.
  template <
      class Ex,
      class Ex_ = std::decay_t<Ex>,
      FOLLY_REQUIRES(IsRegularExceptionType<Ex_>::value)>
  exception_wrapper(std::in_place_t, Ex&& ex);

  template <
      class Ex,
      typename... As,
      FOLLY_REQUIRES(IsRegularExceptionType<Ex>::value)>
  exception_wrapper(std::in_place_type_t<Ex>, As&&... as);

};