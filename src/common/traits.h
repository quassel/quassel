/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

// ---- Function traits --------------------------------------------------------------------------------------------------------------------

namespace detail {

/// @cond DOXYGEN_CANNOT_PARSE_THIS

// Primary template
template<typename Func>
struct FunctionTraitsHelper : public FunctionTraitsHelper<decltype(&Func::operator())>
{};

// Specialization for free function
template<typename R, typename... Args>
struct FunctionTraitsHelper<R(*)(Args...)>
{
    using FunctionType = std::function<R(Args...)>;
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
};

// Specialization for member function with non-const call operator
template<typename C, typename R, typename... Args>
struct FunctionTraitsHelper<R(C::*)(Args...)> : public FunctionTraitsHelper<R(*)(Args...)>
{
    using ClassType = C;
};

// Specialization for member function with const call operator
template<typename C, typename R, typename... Args>
struct FunctionTraitsHelper<R(C::*)(Args...) const> : public FunctionTraitsHelper<R(C::*)(Args...)>
{};

/// @endcond

}  // detail

namespace traits {

/**
 * Provides the class type of the given member function pointer Func.
 */
template<typename Func>
using class_t = typename detail::FunctionTraitsHelper<Func>::ClassType;

/**
 * Provides the return type of the given function type Func.
 */
template<typename Func>
using return_t = typename detail::FunctionTraitsHelper<Func>::ReturnType;

/**
 * Provides the argument types of the given funtion type Func as a tuple.
 */
template<typename Func>
using args_t = typename detail::FunctionTraitsHelper<Func>::ArgsTuple;

/**
 * Provides a std::function type that is compatible to the given function type Func.
 */
template<typename Func>
using function_t = typename detail::FunctionTraitsHelper<Func>::FunctionType;

}  // traits

// ---- decay_t ----------------------------------------------------------------------------------------------------------------------------

namespace detail {

// Primary template
template<typename ...Args>
struct DecayHelper;

// Specialization for std::tuple
template<typename ...Args>
struct DecayHelper<std::tuple<Args...>>
{
    using type = std::tuple<std::decay_t<Args>...>;
};

}  // detail

namespace traits {

/**
 * For the given tuple type T, provides a tuple type that contains the decayed types of T.
 */
template<typename T>
using decay_t = typename detail::DecayHelper<T>::type;

}  // traits
