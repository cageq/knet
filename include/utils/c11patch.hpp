#pragma once   


#include <tuple>
#include <type_traits>
#include <memory>
#include <fmt/format.h>

 

#if __cplusplus <= 201103L

namespace std{

#ifndef __cpp_lib_apply 

  template<size_t... Ints>
        struct index_sequence {
            using type = index_sequence;
            using value_type = size_t;

            static std::size_t size()
            {
                return sizeof...(Ints);
            }
        };

        template<class Sequence1, class Sequence2>
        struct merge_and_renumber;

        template<size_t... I1, size_t... I2>
        struct merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
            : index_sequence<I1..., (sizeof...(I1) + I2)...>
        {};

        template<size_t N>
        struct make_index_sequence
            : merge_and_renumber<typename make_index_sequence<N / 2>::type,
            typename make_index_sequence<N - N / 2>::type>
        {};

        template<>
        struct make_index_sequence<0> : index_sequence<>
        {};

        template<>
        struct make_index_sequence<1> : index_sequence<0>
        {};

        template<typename Func, typename Tuple, std::size_t... index>
        auto apply_helper(Func&& func, Tuple&& tuple, index_sequence<index...>) ->
            decltype(func(std::get<index>(std::forward<Tuple>(tuple))...))
        {
            return func(std::get<index>(std::forward<Tuple>(tuple))...);
        }

        template<typename Func, typename Tuple>
        auto apply(Func&& func, Tuple&& tuple) ->
            decltype(apply_helper(std::forward<Func>(func),
                std::forward<Tuple>(tuple),
                make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
        {
            return apply_helper(std::forward<Func>(func),
                std::forward<Tuple>(tuple),
                make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
        }

#endif 


        template<typename T, typename ...Args>
        std::unique_ptr<T> make_unique(Args&& ...args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        using string_view = fmt::string_view; 
    
}

#endif 