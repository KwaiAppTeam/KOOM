// Author: Qiushi Xue <xueqiushi@kuaishou.com>

#ifndef APM_ASYNC_SAFE_STL_H
#define APM_ASYNC_SAFE_STL_H

#include <async_safe/stack_allocator.h>
#include <deque>
#include <list>
#include <set>

/**
 * async safe stl with stack allocator, e.g.
 *
 * k_string str;
 * k_string_cap<1024> dst;
 * k_ostringstream os;
 */
namespace kwai {
namespace plt {

typedef std::basic_string<char, std::char_traits<char>, StackAllocator<char, 16 * 1024>> k_string;
typedef std::basic_stringstream<char, std::char_traits<char>, StackAllocator<char, 16 * 1024>>
    k_stringstream;
typedef std::basic_ostringstream<char, std::char_traits<char>, StackAllocator<char, 16 * 1024>>
    k_ostringstream;

template <size_t size>
using k_string_cap = std::basic_string<char, std::char_traits<char>, StackAllocator<char, size>>;
template <size_t size>
using k_stringstream_cap =
    std::basic_stringstream<char, std::char_traits<char>, StackAllocator<char, size>>;
template <size_t size>
using k_ostringstream_cap =
    std::basic_ostringstream<char, std::char_traits<char>, StackAllocator<char, size>>;

/**
 * 注意当容器capacity超出StackAllocator的预设值时会回退为default std::allocator
 */
template <class _Kty, class _Pr = std::less<_Kty>, class _Alloc = StackAllocator<_Kty, 1024>>
class k_set : public std::set<_Kty, _Pr, _Alloc> {};

template <class _Kty, class _Pr = std::less<_Kty>, class _Alloc = StackAllocator<_Kty, 1024>>
class k_multiset : public std::multiset<_Kty, _Pr, _Alloc> {};

template <class _Ty, class _Ax = StackAllocator<_Ty, 1024>>
class k_list : public std::list<_Ty, _Ax> {};

template <class _Ty, class _Ax = StackAllocator<_Ty, 64 * 1024>>
class k_vector : public std::vector<_Ty, _Ax> {};

template <class _Ty, class _Ax = StackAllocator<_Ty, 128 * 1024>>
class k_deque : public std::deque<_Ty, _Ax> {};

} // namespace plt

} // namespace kwai

#endif // APM_ASYNC_SAFE_STL_H
