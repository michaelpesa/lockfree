////////////////////////////////////////////////////////////////////////////////
//
// lockfree/compressed_pair.hpp
//
////////////////////////////////////////////////////////////////////////////////

#ifndef LOCKFREE_INCLUDED_46D43F76_9DD7_48C7_B1F7_B3331F1E4011
#define LOCKFREE_INCLUDED_46D43F76_9DD7_48C7_B1F7_B3331F1E4011

#include <type_traits>
#include <tuple>
#include <utility>

namespace lockfree {
namespace detail {

template<typename T, typename Tag>
struct compressed_base_ :
    private T
{
public:
    constexpr compressed_base_()
    noexcept(std::is_nothrow_default_constructible_v<T>) :
        T()
    {}

    constexpr compressed_base_(T const& x)
    noexcept(std::is_nothrow_copy_constructible_v<T>) :
        T(x)
    {}

    constexpr compressed_base_(T&& x)
    noexcept(std::is_nothrow_move_constructible_v<T>) :
        T(std::move(x))
    {}

    template<
        typename... Args,
        typename = std::enable_if_t<std::is_constructible_v<T, Args...>>
    >
    constexpr compressed_base_(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>) :
        T(std::forward<Args>(args)...)
    {}

    compressed_base_& operator=(T const& x) &
    noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        static_cast<T&>(*this) = x;
        return *this;
    }

    compressed_base_& operator=(T&& x) &
    noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        static_cast<T&>(*this) = std::move(x);
        return *this;
    }

    constexpr T const& get() const& noexcept
    { return static_cast<T const&>(*this); }

    constexpr T& get() & noexcept
    { return static_cast<T&>(*this); }

    constexpr T&& get() && noexcept
    { return static_cast<T&&>(*this); }
};

template<typename T, typename Tag>
class compressed_member_
{
public:
    constexpr compressed_member_()
    noexcept(std::is_nothrow_default_constructible_v<T>) :
        member()
    {}

    constexpr compressed_member_(T const& x)
    noexcept(std::is_nothrow_copy_constructible_v<T>) :
        member(x)
    {}

    constexpr compressed_member_(T&& x)
    noexcept(std::is_nothrow_move_constructible_v<T>) :
        member(std::move(x))
    {}

    template<
        typename... Args,
        typename = std::enable_if_t<std::is_constructible_v<T, Args...>>
    >
    constexpr explicit compressed_member_(Args&&... args)
    noexcept(std::is_nothrow_constructible_v<T, Args...>) :
        member(std::forward<Args>(args)...)
    {}

    compressed_member_& operator=(T const& x) &
    noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        member = x;
        return *this;
    }

    compressed_member_& operator=(T&& x) &
    noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        member = std::move(x);
        return *this;
    }

    constexpr T const& get() const& noexcept
    { return member; }

    constexpr T& get() & noexcept
    { return member; }

    constexpr T&& get() && noexcept
    { return std::move(member); }

private:
    T member;
};

template<typename T, typename Tag = void>
using compressed_base =
    std::conditional_t<
        std::is_empty_v<T> && !std::is_final_v<T>,
        detail::compressed_base_<T, Tag>,
        detail::compressed_member_<T, Tag>
    >;


template<typename T1, typename T2>
class compressed_pair :
    private detail::compressed_base<T1, std::integral_constant<int, 1>>,
    private detail::compressed_base<T2, std::integral_constant<int, 2>>
{
public:
    using  first_type = T1;
    using second_type = T2;

    constexpr compressed_pair()
    noexcept(std::is_nothrow_default_constructible_v<T1> &&
             std::is_nothrow_default_constructible_v<T2>) :
        Base1(),
        Base2()
    {}

    constexpr compressed_pair(T1 const& x)
    noexcept(std::is_nothrow_copy_constructible_v<T1> &&
             std::is_nothrow_default_constructible_v<T2>) :
        Base1(x),
        Base2()
    {}

    constexpr compressed_pair(T2 const& y)
    noexcept(std::is_nothrow_default_constructible_v<T1> &&
             std::is_nothrow_copy_constructible_v<T2>) :
        Base1(),
        Base2(y)
    {}

    constexpr compressed_pair(T1 const& x, T2 const& y)
    noexcept(std::is_nothrow_copy_constructible_v<T1> &&
             std::is_nothrow_copy_constructible_v<T2>) :
        Base1(x),
        Base2(y)
    {}

    constexpr compressed_pair(compressed_pair const& x)
    noexcept(std::is_nothrow_copy_constructible_v<T1> &&
             std::is_nothrow_copy_constructible_v<T2>) :
        Base1(x.first()),
        Base2(x.second())
    {}

    constexpr compressed_pair(compressed_pair&& x)
    noexcept(std::is_nothrow_move_constructible_v<T1> &&
             std::is_nothrow_move_constructible_v<T2>) :
        Base1(std::move(x.first())),
        Base2(std::move(x.second()))
    {}

    template<
        typename U1,
        typename U2,
        typename = std::enable_if_t<
            std::is_convertible_v<U1, T1> &&
            std::is_convertible_v<U2, T2>
        >
    >
    constexpr compressed_pair(U1&& u1, U2&& u2)
    noexcept(std::is_nothrow_constructible_v<T1, U1> &&
             std::is_nothrow_constructible_v<T2, U2>) :
        Base1(std::forward<U1>(u1)),
        Base2(std::forward<U2>(u2))
    {}

    template<
        typename U1,
        typename U2,
        typename = std::enable_if_t<
            std::is_convertible_v<U1 const&, T1> &&
            std::is_convertible_v<U2 const&, T2>
        >
    >
    constexpr compressed_pair(compressed_pair<U1, U2> const& p)
    noexcept(std::is_nothrow_constructible_v<T1, U1 const&> &&
             std::is_nothrow_constructible_v<T2, U2 const&>) :
        Base1(p.first()),
        Base2(p.second())
    {}

    template<
        typename U1,
        typename U2,
        typename = std::enable_if_t<
            std::is_convertible_v<U1, T1> &&
            std::is_convertible_v<U2, T2>
        >
    >
    constexpr compressed_pair(compressed_pair<U1, U2>&& p)
    noexcept(std::is_nothrow_constructible_v<T1, U1> &&
             std::is_nothrow_constructible_v<T2, U2>) :
        Base1(std::forward<U1>(p.first())),
        Base2(std::forward<U2>(p.second()))
    {}

    template<typename... Args1, typename... Args2>
    constexpr compressed_pair(std::piecewise_construct_t,
                              std::tuple<Args1...> args1,
                              std::tuple<Args2...> args2)
    noexcept(std::is_nothrow_constructible_v<T1, Args1...> &&
             std::is_nothrow_constructible_v<T2, Args2...>) :
        compressed_pair(std::piecewise_construct,
                        std::move(args1),
                        std::move(args2),
                        std::make_index_sequence<sizeof...(Args1)>(),
                        std::make_index_sequence<sizeof...(Args2)>())
    {}

    compressed_pair& operator=(compressed_pair const& x) &
    noexcept(std::is_nothrow_copy_assignable_v<T1> &&
             std::is_nothrow_copy_assignable_v<T2>)
    {
        first()  = x.first();
        second() = x.second();
        return *this;
    }

    compressed_pair& operator=(compressed_pair&& x) &
    noexcept(std::is_nothrow_move_assignable_v<T1> &&
             std::is_nothrow_move_assignable_v<T2>)
    {
        first()  = std::forward<T1>(x.first());
        second() = std::forward<T2>(x.second());
        return *this;
    }

    constexpr first_type const& first() const& noexcept
    { return static_cast<Base1 const&>(*this).get(); }

    constexpr first_type& first() & noexcept
    { return static_cast<Base1&>(*this).get(); }

    constexpr first_type&& first() && noexcept
    { return static_cast<Base1&&>(*this).get(); }

    constexpr second_type const& second() const& noexcept
    { return static_cast<Base2 const&>(*this).get(); }

    constexpr second_type& second() & noexcept
    { return static_cast<Base2&>(*this).get(); }

    constexpr second_type&& second() && noexcept
    { return static_cast<Base2&&>(*this).get(); }

    void swap(compressed_pair& x)
    noexcept(std::is_nothrow_swappable_v<T1> &&
             std::is_nothrow_swappable_v<T2>)
    {
        using std::swap;
        swap(first(),  x.first());
        swap(second(), x.second());
    }

private:
    using Base1 = detail::compressed_base<T1, std::integral_constant<int, 1>>;
    using Base2 = detail::compressed_base<T2, std::integral_constant<int, 2>>;

    template<
        typename... Args1,
        typename... Args2,
        std::size_t... I1,
        std::size_t... I2
    >
    constexpr compressed_pair(std::piecewise_construct_t,
                              std::tuple<Args1...> args1,
                              std::tuple<Args2...> args2,
                              std::index_sequence<I1...>,
                              std::index_sequence<I2...>)
    noexcept(std::is_nothrow_constructible_v<T1, Args1...> &&
             std::is_nothrow_constructible_v<T2, Args2...>) :
        Base1(std::forward<Args1>(std::get<I1>(args1))...),
        Base2(std::forward<Args2>(std::get<I2>(args2))...)
    {}

    constexpr friend bool operator==(compressed_pair const& x,
                                     compressed_pair const& y)
    {
        return x.first() == y.first() && x.second() == y.second();
    }

    constexpr friend bool operator<(compressed_pair const& x,
                                    compressed_pair const& y)
    {
        return x.first() < y.first() ||
            (!(y.first() < x.first()) && (x.second() < y.second()));
    }
};


template<typename T1, typename T2>
void swap(compressed_pair<T1, T2>& x, compressed_pair<T1, T2>& y)
noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template<typename T1, typename T2>
constexpr auto make_compressed_pair(T1&& x, T2&& y) ->
    compressed_pair<std::decay_t<T1>, std::decay_t<T2>>
{
    return {std::forward<T1>(x), std::forward<T2>(y)};
}

}}      // namespace lockfree::detail

#endif  // LOCKFREE_INCLUDED_46D43F76_9DD7_48C7_B1F7_B3331F1E4011
