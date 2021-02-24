#pragma once   

#include <tuple>
#include <type_traits>
#include <memory>
#include <fmt/format.h>

 
#ifndef   CPLUSPLUSVER
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define CPLUSPLUSVER  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define CPLUSPLUSVER  __cplusplus
# endif
#endif



#define KNET_CPP98_OR_GREATER  ( CPLUSPLUSVER >= 199711L )
#define KNET_CPP11_OR_GREATER  ( CPLUSPLUSVER >= 201103L )
#define KNET_CPP14_OR_GREATER  ( CPLUSPLUSVER >= 201402L )
#define KNET_CPP17_OR_GREATER  ( CPLUSPLUSVER >= 201703L )
#define KNET_CPP20_OR_GREATER  ( CPLUSPLUSVER >= 202000L )

namespace std{

#if CPLUSPLUSVER <= 201103L 

#ifndef __cpp_lib_apply 

  template<size_t... Ints>
        struct apply_index_sequence {
            using type = apply_index_sequence;
            using value_type = size_t;

            static std::size_t size()
            {
                return sizeof...(Ints);
            }
        };

        template<class Sequence1, class Sequence2>
        struct merge_and_renumber;

        template<size_t... I1, size_t... I2>
        struct merge_and_renumber<apply_index_sequence<I1...>, apply_index_sequence<I2...>>
            : apply_index_sequence<I1..., (sizeof...(I1) + I2)...>
        {};

        template<size_t N>
        struct apply_make_index_sequence
            : merge_and_renumber<typename apply_make_index_sequence<N / 2>::type,
            typename apply_make_index_sequence<N - N / 2>::type>
        {};

        template<>
        struct apply_make_index_sequence<0> : apply_index_sequence<>
        {};

        template<>
        struct apply_make_index_sequence<1> : apply_index_sequence<0>
        {};

        template<typename Func, typename Tuple, std::size_t... index>
        auto apply_helper(Func&& func, Tuple&& tuple, apply_index_sequence<index...>) ->
            decltype(func(std::get<index>(std::forward<Tuple>(tuple))...))
        {
            return func(std::get<index>(std::forward<Tuple>(tuple))...);
        }

        template<typename Func, typename Tuple>
        auto apply(Func&& func, Tuple&& tuple) ->
            decltype(apply_helper(std::forward<Func>(func),
                std::forward<Tuple>(tuple),
                apply_make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
        {
            return apply_helper(std::forward<Func>(func),
                std::forward<Tuple>(tuple),
                apply_make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
        }
 

#endif //__cpp_lib_apply

   
  template<typename T, typename ...Args>
        std::unique_ptr<T> make_unique(Args&& ...args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

#if __has_include( <string_view> )   

#ifdef  __cpp_lib_string_view
		#include <string_view>
#else 
        using string_view = fmt::string_view; 
#endif 

#else 
        using string_view = fmt::string_view; 
#endif  // __has_include 



#endif  //CPLUSPLUSVER < 201103L 


} //namespace std


