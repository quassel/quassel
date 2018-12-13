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

#include <boost/optional.hpp>

#include <QDebug>
#include <QVariant>
#include <QVariantList>

#include "traits.h"

// ---- Invoke function with argument list -------------------------------------------------------------------------------------------------

namespace detail {

// Helper for invoking the callable, wrapping its return value in a QVariant (default-constructed if callable returns void).
// The correct overload is selected via SFINAE.
template<typename Callable, typename ...Args>
auto invokeWithArgs(const Callable& c, Args&&... args)
    -> std::enable_if_t<std::is_void<traits::return_t<Callable>>::value, QVariant>
{
    c(std::forward<Args>(args)...);
    return QVariant{};
}

template<typename Callable, typename ...Args>
auto invokeWithArgs(const Callable& c, Args&&... args)
    -> std::enable_if_t<!std::is_void<traits::return_t<Callable>>::value, QVariant>
{
    return QVariant::fromValue(c(std::forward<Args>(args)...));
}

// Helper for unpacking the argument list via an index sequence
template<typename Callable, std::size_t ...Is, typename ArgsTuple = traits::args_t<Callable>>
boost::optional<QVariant> invokeWithArgsList(const Callable& c, const QVariantList& args, std::index_sequence<Is...>)
{
    // Sanity check that all types can be converted
    std::array<bool, std::tuple_size<ArgsTuple>::value> convertible{{args[Is].canConvert<std::decay_t<std::tuple_element_t<Is, ArgsTuple>>>()...}};
    for (size_t i = 0; i < convertible.size(); ++i) {
        if (!convertible[i]) {
            qWarning() << "Cannot convert parameter" << i << "from type" << args[static_cast<int>(i)].typeName() << "to expected argument type";
            return boost::none;
        }
    }

    // Invoke callable with unmarshalled arguments
    return invokeWithArgs(c, args[Is].value<std::decay_t<std::tuple_element_t<Is, ArgsTuple>>>()...);
}

}  // detail

/**
 * Invokes the given callable with the arguments contained in the given variant list.
 *
 * The types contained in the given QVariantList are converted to the types expected by the callable.
 * If the invocation is successful, the returned optional contains a QVariant with the return value,
 * or an invalid QVariant if the callable returns void.
 * If the conversion fails, or if the argument count does not match, this function returns boost::none
 * and the callable is not invoked.
 *
 * @param c    Callable
 * @param args Arguments to be given to the callable
 * @returns An optional containing a QVariant with the return value if the callable could be invoked with
 *          the given list of arguments; otherwise boost::none
 */
template<typename Callable>
boost::optional<QVariant> invokeWithArgsList(const Callable& c, const QVariantList& args)
{
    constexpr auto tupleSize = std::tuple_size<traits::args_t<Callable>>::value;

    if (tupleSize != args.size()) {
        qWarning().nospace() << "Argument count mismatch! Expected: " << tupleSize << ", actual: " << args.size();
        return boost::none;
    }
    return detail::invokeWithArgsList(c, args, std::make_index_sequence<tupleSize>{});
}

/**
 * Invokes the given member function pointer on the given object with the arguments contained in the given variant list.
 *
 * The types contained in the given QVariantList are converted to the types expected by the member function.
 * If the invocation is successful, the returned optional contains a QVariant with the return value,
 * or an invalid QVariant if the member function returns void.
 * If the conversion fails, or if the argument count does not match, this function returns boost::none
 * and the member function is not invoked.
 *
 * @param c    Callable
 * @param args Arguments to be given to the member function
 * @returns An optional containing a QVariant with the return value if the member function could be invoked with
 *          the given list of arguments; otherwise boost::none
 */
template<typename R, typename C, typename ...Args>
boost::optional<QVariant> invokeWithArgsList(C* object, R(C::*func)(Args...), const QVariantList& args)
{
    if (!object) {
        qWarning() << "Cannot invoke member function on a null object!";
        return boost::none;
    }
    if (sizeof...(Args) != args.size()) {
        qWarning().nospace() << "Argument count mismatch! Expected: " << sizeof...(Args) << ", actual: " << args.size();
        return boost::none;
    }
    return detail::invokeWithArgsList([object, func](Args&&... args) {
        return (object->*func)(std::forward<decltype(args)>(args)...);
    }, args, std::make_index_sequence<sizeof...(Args)>{});
}
