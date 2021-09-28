/*
 * Copyright (c) 2020. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by Qiushi Xue <xueqiushi@kuaishou.com> on 2020.
 *
 */

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
 * Note that when the container capacity exceeds the default value of StackAllocator, it will
 * fall back to default std::allocator.
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
