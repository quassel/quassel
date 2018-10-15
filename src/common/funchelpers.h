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

#include <array>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <QDebug>
#include <QVariantList>

// ---- Function traits --------------------------------------------------------------------------------------------------------------------

namespace detail {

/// @cond DOXYGEN_CANNOT_PARSE_THIS

// Primary template
template<typename Func>
struct FuncHelper : public FuncHelper<decltype(&Func::operator())>
{};

// Overload for free function
template<typename R, typename... Args>
struct FuncHelper<R(*)(Args...)>
{
    using FunctionType = std::function<R(Args...)>;
    using ReturnType = R;
    using ArgsTuple = std::tuple<Args...>;
};

// Overload for member function with non-const call operator
template<typename C, typename R, typename... Args>
struct FuncHelper<R(C::*)(Args...)> : public FuncHelper<R(*)(Args...)>
{
    using ClassType = C;
};

// Overload for member function with const call operator
template<typename C, typename R, typename... Args>
struct FuncHelper<R(C::*)(Args...) const> : public FuncHelper<R(C::*)(Args...)>
{};

/// @endcond

}  // namespace detail

/**
 * Provides traits for the given callable.
 */
template<typename Callable>
using FunctionTraits = detail::FuncHelper<Callable>;

// ---- Invoke function with argument list -------------------------------------------------------------------------------------------------

namespace detail {

// Helper for unpacking the argument list via an index sequence
template<typename Callable, std::size_t ...Is, typename ArgsTuple = typename FunctionTraits<Callable>::ArgsTuple>
bool invokeWithArgsList(const Callable& c, const QVariantList& args, std::index_sequence<Is...>)
{
    // Sanity check that all types can be converted
    std::array<bool, std::tuple_size<ArgsTuple>::value> convertible{{args[Is].canConvert<std::decay_t<std::tuple_element_t<Is, ArgsTuple>>>()...}};
    for (size_t i = 0; i < convertible.size(); ++i) {
        if (!convertible[i]) {
            qWarning() << "Cannot convert parameter" << i << "from type" << args[static_cast<int>(i)].typeName() << "to expected argument type";
            return false;
        }
    }

    // Invoke callable
    c(args[Is].value<std::decay_t<std::tuple_element_t<Is, ArgsTuple>>>()...);
    return true;
}

}  // detail

/**
 * Invokes the given callable with the arguments contained in the given variant list.
 *
 * The types contained in the given QVariantList are converted to the types expected by the callable.
 * If the conversion fails, or if the argument count does not match, this function returns false and
 * the callable is not invoked.
 *
 * @param c    Callable
 * @param args Arguments to be given to the callable
 * @returns true if the callable could be invoked with the given list of arguments
 */
template<typename Callable>
bool invokeWithArgsList(const Callable& c, const QVariantList& args)
{
    using ArgsTuple = typename FunctionTraits<Callable>::ArgsTuple;
    constexpr auto tupleSize = std::tuple_size<ArgsTuple>::value;

    if (tupleSize != args.size()) {
        qWarning().nospace() << "Argument count mismatch! Expected: " << tupleSize << ", actual: " << args.size();
        return false;
    }
    return detail::invokeWithArgsList(c, args, std::make_index_sequence<tupleSize>{});
}
