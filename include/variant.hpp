//
// Created by akarpovskii on 19.09.17.
//

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <sstream>

namespace my_variant {
    namespace details {
        template<size_t a, size_t... args>
        struct static_max;

        template<size_t a>
        struct static_max<a> {
            static const size_t value = a;
        };

        template<size_t a, size_t b, size_t... args>
        struct static_max<a, b, args...> {
            static const size_t value = a >= b ? static_max<a, args...>::value :
                                        static_max<b, args...>::value;
        };

        template<typename T, typename... Ts>
        struct is_one_of {
            static constexpr bool value = std::disjunction<std::is_same<T, Ts>...>::value;
        };

        /// @value is equal to true if static_cast<To>(From) is available
        template<typename From, typename To>
        struct is_explicitly_convertible {
            template<typename F, typename T>
            static constexpr auto test(int) -> decltype(static_cast<T>(std::declval<F>()), true) {
                return true;
            };

            template<typename F, typename T>
            static constexpr auto test(...) -> bool {
                return false;
            };

            static bool const value = test<From, To>(0);
        };

        template<typename Obj>
        struct is_streamable {
            template<typename O>
            static constexpr auto
            test(int) -> decltype(std::declval<std::ostream &>() << std::declval<const O &>(), true) {
                return true;
            };

            template<typename O>
            static constexpr auto test(...) -> bool {
                return false;
            };

            static bool const value = test<Obj>(0);
        };

        template<typename From, typename To>
        struct is_convertible_helper {
            static bool const value = is_explicitly_convertible<From, To>::value;
        };

        template<typename From>
        struct is_convertible_helper<From, std::string> {
            static bool const value = is_explicitly_convertible<From, std::string>::value ||
                                      is_streamable<From>::value;
        };

        template<typename From, typename To>
        struct cast_helper {
            template<typename F = From, typename T = To>
            static typename std::enable_if<is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                return static_cast<T>(from);
            }

            template<typename F = From, typename T = To>
            static typename std::enable_if<!is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                throw std::bad_cast();
            }
        };

        template<typename From, bool IsExplicitlyConvertible>
        struct cast_to_string_helper;

        template<typename From>
        struct cast_to_string_helper<From, true> {
            template<typename F = From, typename T = std::string>
            static typename std::enable_if<is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                return static_cast<T>(from);
            }

            template<typename F = From, typename T = std::string>
            static typename std::enable_if<!is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                throw std::bad_cast();
            }
        };

        template<typename From>
        struct cast_to_string_helper<From, false> {
            template<typename F = From, typename T = std::string>
            static typename std::enable_if<is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                std::stringstream ss;
                ss << from;
                return ss.str();
            }

            template<typename F = From, typename T = std::string>
            static typename std::enable_if<!is_convertible_helper<F, T>::value, T>::type
            cast(const F &from) {
                throw std::bad_cast();
            }
        };

        template<typename From>
        struct cast_helper<From, std::string> {
            static std::string cast(const From &from) {
                return cast_to_string_helper<From, is_explicitly_convertible<From, std::string>::value>::cast(from);
            }
        };

        template<typename... Ts>
        class helper;

        template<typename F, typename... Ts>
        class helper<F, Ts...> {
        public:
            /// @return type_id
            static size_t initialize(void *data) {
                new(data) F();
                return typeid(F).hash_code();
            }

            static void destroy(size_t type_id, void *data) {
                if (type_id == typeid(F).hash_code()) {
                    reinterpret_cast<F *>(data)->~F();
                } else {
                    helper<Ts...>::destroy(type_id, data);
                }
            }

            static void copy(size_t type_id, void *src, const void *dst) {
                if (type_id == typeid(F).hash_code()) {
                    new(dst) F(*reinterpret_cast<const F *>(src));
                } else {
                    helper<Ts...>::copy(type_id, src, dst);
                }
            }

            static void move(size_t type_id, void *src, void *dst) {
                if (type_id == typeid(F).hash_code()) {
                    new(dst) F(std::move(*reinterpret_cast<F *>(src)));
                } else {
                    helper<Ts...>::move(type_id, src, dst);
                }
            }

            static bool is_equal(size_t type_id, const void *a, const void *b) {
                if (type_id == typeid(F).hash_code()) {
                    return *reinterpret_cast<const F *>(a) == *reinterpret_cast<const F *>(b);
                } else {
                    return helper<Ts...>::is_equal(type_id, a, b);
                }
            }

            /// @return index of current type held
            static size_t index(size_t type_id, size_t start = 0) {
                if (type_id == typeid(F).hash_code()) {
                    return start;
                } else {
                    return helper<Ts...>::index(type_id, start + 1);
                }
            }

            template<typename To>
            static bool is_convertible(size_t type_id) {
                if (type_id == typeid(F).hash_code()) {
                    return is_convertible_helper<F, To>::value;
                } else {
                    return helper<Ts...>::template is_convertible<To>(type_id);
                }
            }

            template<typename To>
            static To cast(size_t type_id, const void *data) {
                if (type_id == typeid(F).hash_code()) {
                    return cast_helper<F, To>::cast(*reinterpret_cast<const F *>(data));
                } else {
                    return helper<Ts...>::template cast<To>(type_id, data);
                }
            }
        };

        template<>
        class helper<> {
        public:
            static void destroy(size_t type_id, void *data) {}

            static void copy(size_t type_id, void *src, const void *dst) {}

            static void move(size_t type_id, void *src, void *dst) {}

            [[noreturn]] static bool is_equal(size_t type_id, const void *a, const void *b) {
                throw std::logic_error("Function should not return");
            }

            [[noreturn]] static size_t index(size_t type_id, size_t start = 0) {
                throw std::logic_error("Function should not return");
            }

            template<typename To>
            [[noreturn]] static bool is_convertible(size_t type_id) {
                throw std::logic_error("Function should not return");
            }

            template<typename To>
            [[noreturn]] static To cast(size_t type_id, const void *data) {
                throw std::logic_error("Function should not return");
            }
        };
    }

    template<typename... Ts>
    class variant {
    public:
        variant() {
            type_id = details::helper<Ts...>::initialize(&data);
        }

        variant(const variant &that)
                : type_id(that.type_id) {
            details::helper<Ts...>::copy(type_id, &that.data, &data);
        }

        template<typename T>
        using is_one_of_remove_ref = details::is_one_of<typename std::remove_reference<T>::type, Ts...>;

        template<typename T,
                typename = typename std::enable_if<is_one_of_remove_ref<T>::value, void>::type>
        variant(T &&v) {
            set<T>(std::forward<T>(v));
        }

        variant(variant &&that) noexcept
                : type_id(std::move(type_id)) {
            details::helper<Ts...>::move(type_id, &that.data, &data);
        }

        virtual ~variant() {
            details::helper<Ts...>::destroy(type_id, &data);
        }

        variant &operator=(const variant &that) {
            if (*this != that) {
                type_id = that.type_id;
                details::helper<Ts...>::copy(type_id, &that.data, &data);
            }
            return *this;
        }

        variant &operator=(variant &&that) noexcept {
            if (*this != that) {
                type_id = std::move(that.type_id);
                details::helper<Ts...>::move(type_id, &that.data, &data);
            }
            return *this;
        }

        template<typename T,
                typename = typename std::enable_if<is_one_of_remove_ref<T>::value, void>::type>
        variant &operator=(T &&v) {
            set<T>(std::forward<T>(v));
            return *this;
        }

        template<typename T>
        bool holds_alternative() const {
            return type_id == typeid(T).hash_code();
        }

        template<typename T, typename... Args,
                typename = typename std::enable_if<is_one_of_remove_ref<T>::value, void>::type>
        void set(Args &&... args) {
            details::helper<Ts...>::destroy(type_id, &data);
            new(&data) typename std::remove_reference<T>::type(std::forward<Args>(args)...);
            type_id = typeid(T).hash_code();
        };

        template<typename T,
                typename = typename std::enable_if<is_one_of_remove_ref<T>::value, void>::type>
        T &get() {
            if (type_id == typeid(T).hash_code()) {
                return *reinterpret_cast<T *>(&data);
            } else {
                throw std::bad_cast();
            }
        }

        size_t index() const {
            return details::helper<Ts...>::index(type_id);
        }

        bool operator==(const variant &that) const {
            return type_id == that.type_id &&
                   details::helper<Ts...>::is_equal(type_id, &data, &that.data);
        }

        bool operator!=(const variant &that) const {
            return !(*this == that);
        }

        /// @return true if static_cast<To> is available
        template<typename To>
        bool can_convert() const {
            return details::helper<Ts...>::template is_convertible<To>(type_id);
        }

        /// @return object with requested type
        /// @throw std::bad_cast if cast is unavailable
        template<typename To>
        To convert() const {
            return details::helper<Ts...>::template cast<To>(type_id, &data);
        }

    protected:
        static const size_t data_size = details::static_max<sizeof(Ts)...>::value;

        static const size_t data_align = details::static_max<alignof(Ts)...>::value;

        using data_t = typename std::aligned_storage<data_size, data_align>::type;

        size_t type_id;

        data_t data;
    };
}