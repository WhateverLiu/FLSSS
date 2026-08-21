#pragma once
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>


template <class TupleLike>
constexpr std::size_t tuple_size(const TupleLike&) {
    return std::tuple_size_v<std::remove_reference_t<TupleLike>>;
}


template <std::size_t N, class Func>
constexpr void static_for(Func&& func) {
    [&]<std::size_t... i>(std::index_sequence<i...>) {
        (func(std::integral_constant<std::size_t, i>{}), ...);
    }(std::make_index_sequence<N>{});
}
/*
usage example:
static_for<field_names.size()>([&](auto i) {
    result[field_names[i]] = polarize(
        std::get<i>(reduced),
        std::array{field_names[i]}
    );
});
*/


template <class TupleLike, class Func, std::size_t... i>
constexpr auto static_map_impl(
    TupleLike&& tuple_like,
    Func && func,
    std::index_sequence<i...>
) {
    return std::make_tuple(
        func(
            std::get<i>(std::forward<TupleLike>(tuple_like)),
            std::integral_constant<std::size_t, i>{}
        )...
    );
}


template <class TupleLike, class Func>
constexpr auto static_map(TupleLike&& tuple_like, Func&& func) {
    constexpr std::size_t N = tuple_size(tuple_like);
    return static_map_impl(
        std::forward<TupleLike>(tuple_like),
        std::forward<Func>(func),
        std::make_index_sequence<N>{}
    );
}

/*
auto indexes = index_tuple(std::make_index_sequence<5>{});
auto mapped = static_map(indexes, [](auto index, auto i) {
    return index * 10;
});
*/
