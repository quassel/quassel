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

namespace detail {

/// @cond DOXYGEN_CANNOT_PARSE_THIS

// Primary template
template<typename Func>
struct FuncHelper : public FuncHelper<decltype(&Func::operator())>
{};

// Overload for member function with const call operator
template<typename C, typename R, typename... Args>
struct FuncHelper<R (C::*)(Args...) const> : public FuncHelper<R (C::*)(Args...)>
{};

// Overload for member function with non-const call operator
template<typename C, typename R, typename... Args>
struct FuncHelper<R (C::*)(Args...)>
{
    using ClassType = C;
    using FunctionType = std::function<R(Args...)>;
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
};

/// @endcond

}  // namespace detail

/**
 * Provides traits for the given callable.
 */
template<typename Callable>
using MemberFunction = detail::FuncHelper<Callable>;
