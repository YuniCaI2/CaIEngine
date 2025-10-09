#pragma once
#include <type_traits>

template<class T>
struct variable_traits {
    using value_type = std::remove_cv_t<T>;
    static constexpr bool is_pointer = false;
    static constexpr bool is_member_object_pointer = false;
};

template<class T>
struct variable_traits<T*> {
    using pointer_type = T*;
    using value_type = std::remove_cv_t<T>; //去除const和volatile
    static constexpr bool is_pointer = true;
    static constexpr bool is_member_object_pointer = false;
};

template<class C, class T>
struct variable_traits<T C::*> {
    static_assert(std::is_member_object_pointer_v<T C::*>,
                  "Only member object pointers are supported (not member functions).");
    using pointer_type = T C::*;
    using class_type   = C;
    using value_type   = std::remove_cv_t<T>;
    static constexpr bool is_pointer = false;
    static constexpr bool is_member_object_pointer = true;
};

template<class T>
using variable_value_t = typename variable_traits<T>::value_type;
