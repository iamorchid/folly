/*
 * 这个测试需要在Linux环境下的gcc进行 (macos上不符合预期): 
 * 1) gcc -std=c++17 -lstdc++ -DTEST_LVALUE -DTEST_1 -DTEST_2 TemplateTest.cpp 
 * 2) gcc -std=c++17 -lstdc++ -DTEST_LVALUE TemplateTest.cpp 
 * 3) gcc -std=c++17 -lstdc++ -DTEST_RVALUE TemplateTest.cpp
 * 
 * references:
 * https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers
 */
#include <iostream>
#include <type_traits>

using namespace std;

class Student {
public:
  Student() = default;

  explicit Student(const Student& s) noexcept {
    std::cout << "Student(Student& s)" << std::endl;
  }

  explicit Student(Student&& s) {
    std::cout << "Student(Student&& s)" << std::endl;
  }
};

namespace chuzhi {

template <class _Tp>
constexpr _Tp&& forward(typename remove_reference<_Tp>::type&& __t) noexcept {
  static_assert(!is_lvalue_reference<_Tp>::value, "cannot forward an rvalue as an lvalue");
  return static_cast<_Tp&&>(__t);
}

}

// T&& 表示万能引用, 即T会保留原始参数的引用类型, 即传入左值, T就是左值类型 (此时, T为Student&);
// 反之, 即传入右值, T就是右值类型(此时, T为Student).
template <typename T>
void test(T&& s) {
  using DS = std::decay_t<T>;

    if constexpr (std::is_same_v<T, Student&>) {
        std::cout << "T type: Student&" << std::endl;
    } else if constexpr (std::is_same_v<T, Student&&>) {
        std::cout << "T type: Student" << std::endl;
    } else if constexpr (std::is_same_v<T, Student>) {
        std::cout << "T type: Student" << std::endl;
    }

  std::cout << "left value : " << std::is_lvalue_reference<T>::value << std::endl;
  std::cout << "right value: " << std::is_rvalue_reference<T>::value << std::endl;
  std::cout << "left value : " << std::is_lvalue_reference<T&&>::value << std::endl;
  std::cout << "right value: " << std::is_rvalue_reference<T&&>::value << std::endl;

  // 函数范围内, s本身就是左值
  std::cout << noexcept(DS(s)) << std::endl; 

  // 完美转发, 虽然s是左值, 但forward会基于T, 将s转成对应左值或者右值
  std::cout << noexcept(DS(std::forward<T>(s))) << std::endl; 

#if defined(TEST_1) || defined(TEST_RVALUE)
  // 对于T是左值引用类型是, 传入右值, 这里会触发static_assert (不能将右值转成左值)
  std::cout << noexcept(DS(std::forward<T>(Student()))) << std::endl;
#endif

  // std::forward有支持左值或者右值参数的不同模版
  std::cout << noexcept(DS(std::forward<T>(std::declval<T>()))) << std::endl; 

#if defined(TEST_2) || defined(TEST_RVALUE)
  // T为左值引用类型时, T&& 对应的还是左值引用类型, 此时chuzhi::forward参数不匹配
  // std::declval<T>(): The return type is T&& unless T is (possibly cv-qualified) void, in which case the return type is T.
  std::cout << noexcept(DS(chuzhi::forward<T>(std::declval<T>()))) << std::endl;
#endif
  
  std::cout << noexcept(DS(std::declval<T>())) << std::endl;
  std::cout << noexcept(DS(std::declval<T&&>())) << std::endl;

  // static_cast<T&&>(s)和完美转发是一样的效果, 即保持传入参数的原始类型
  std::cout << noexcept(DS(static_cast<T&&>(s))) << std::endl;

  std::cout << noexcept(DS(static_cast<T>(s))) << std::endl;

  DS s1(static_cast<T&&>(s));
  DS s2(static_cast<T>(s));
}

template <typename T1, typename T2>
struct Bundle2 {
  typedef T1 First;
  typedef T2 Second;
};

template <typename T>
struct Bundle3 {
  typedef T First;
  typedef Bundle2<T&, T&&> B2;
  typedef typename B2::First Second;
  typedef typename B2::Second Three;
};

int main() {
  std::cout << "left value : " << std::is_lvalue_reference<typename Bundle3<int>::Second>::value << std::endl;
  std::cout << "right value: " << std::is_rvalue_reference<typename Bundle3<int>::Three>::value << std::endl;
  std::cout << "left value : " << std::is_lvalue_reference<typename Bundle3<int&>::Second>::value << std::endl;
  std::cout << "right value: " << std::is_rvalue_reference<typename Bundle3<int&>::Three>::value << std::endl;
  std::cout << "left value : " << std::is_lvalue_reference<typename Bundle3<int&&>::Second>::value << std::endl;
  std::cout << "right value: " << std::is_rvalue_reference<typename Bundle3<int&&>::Three>::value << std::endl;

#if defined(TEST_LVALUE)
  Student s;
  std::cout << "--------------" << std::endl;
  test(s);
#endif

#if defined(TEST_RVALUE)
  std::cout << "--------------" << std::endl;
  test(Student());
#endif
}