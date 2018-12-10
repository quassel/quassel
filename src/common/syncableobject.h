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

#include "common-export.h"

#include <array>
#include <unordered_map>
#include <type_traits>

#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>

#include <QByteArray>
#include <QDataStream>
#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

#include "invoke.h"
#include "protocol.h"
#include "signalproxy.h"
#include "traits.h"
#include "types.h"
#include "util.h"

/**
 * This macro needs to be declared next to the Q_OBJECT macro in every class directly inheriting from SyncableObject.
 *
 * The class name must be specified as the first parameter, followed by the names of all methods that should be invokable
 * by sync calls (including receive methods that are intended to receive the return value of a matching request method).
 *
 * @code
 * SYNCABLE_OBJECT(Class, syncMethod0, syncMethod1, ...)
 * @endcode
 *
 * @note: Specializations of a base syncable object for core/client must not use the macro;
 *        i.e., if you have Foo, ClientFoo and/or CoreFoo, the SYNCABLE_OBJECT macro would
 *        only be declared in the class declaration of Foo.
 */
#define SYNCABLE_OBJECT(...)                                                                                                                     \
public:                                                                                                                                          \
    const QMetaObject* syncMetaObject() const final override { return &staticMetaObject; }                                                       \
private:                                                                                                                                         \
    static constexpr auto syncMethodNames() { return buildSyncMethodNames(SYNCOBJ_APPLY(SYNCOBJ_STRINGIZE, __VA_ARGS__, SYNCOBJ_BASEMETHODS)); } \
    static constexpr auto syncMethodPointers() { return std::make_tuple(SYNCOBJ_APPLY(SYNCOBJ_MAKE_PTR, __VA_ARGS__, SYNCOBJ_BASEMETHODS)); }    \
    static constexpr bool isSyncMethodDeclared(const char* syncMethod) { return syncMethodIndex(syncMethod, syncMethodNames()) >= 0; }           \
                                                                                                                                                 \
    SyncMethodMap syncMethodMap() const final override { return buildSyncMethodMap(syncMethodNames(), syncMethodPointers()); }                   \


#define SYNC(...) sync_call__(SignalProxy::Server, __func__, __VA_ARGS__);
#define REQUEST(...) sync_call__(SignalProxy::Client, __func__, __VA_ARGS__);

#define SYNC_OTHER(x, ...) sync_call__(SignalProxy::Server, #x, __VA_ARGS__);
#define REQUEST_OTHER(x, ...) sync_call__(SignalProxy::Client, #x, __VA_ARGS__);

#define ARG(x) const_cast<void*>(reinterpret_cast<const void*>(&x))
#define NO_ARG 0

// ---- Internal macros that should not be used directly-----------------------------------------------------------------------------------

/// Declares the sync methods of the base SyncableObject.
#define SYNCOBJ_BASEMETHODS update, requestUpdate

/// Applies the given operation to each of the variadic arguments except the first one, and produces a comma-separated list of the results.
/// The first variadic argument holds the class name. The case where it is the only argument is handled by performing no operation.
#define SYNCOBJ_APPLY(Op, ...) \
    BOOST_PP_REMOVE_PARENS(BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), 1), (SYNCOBJ_APPLY_IMPL(Op, __VA_ARGS__, _)), ()))

/// Applies the given operation with the given class name to each of the variadic arguments. Expects a dummy argument at the end of the
/// list of variadic arguments, the result for which is discarded in the end. This is to handle the special case of no actual arguments
/// (even though SYNCOBJ_APPLY has a special case for this and thus does not call this macro in this case, some compilers still expect
/// __VA_ARGS__ to be non-empty).
#define SYNCOBJ_APPLY_IMPL(Op, Class, ...) \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_POP_BACK(BOOST_PP_SEQ_TRANSFORM(Op, Class, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))))

/// Operation that creates a function pointer. The first argument s is a dummy needed for implementation reasons.
#define SYNCOBJ_MAKE_PTR(s, Class, Func) &Class::Func

/// Operation that stringizes the given function name. s and t are dummies needed for implementation reasons.
#define SYNCOBJ_STRINGIZE(s, t, Func) #Func

// ---- SyncableObject declaration --------------------------------------------------------------------------------------------------------

class COMMON_EXPORT SyncableObject : public QObject
{
    Q_OBJECT

protected:
    template<std::size_t N>
    using SyncMethodNames = std::array<const char*, N>;

    using SyncMethodWrapper = std::function<boost::optional<Protocol::SyncMessage>(SyncableObject*, const QVariantList&)>;
    using SyncMethodMap = std::unordered_map<QByteArray, SyncMethodWrapper, Hash<QByteArray>>;

public:
    SyncableObject(QObject* parent = nullptr);
    SyncableObject(const QString& objectName, QObject* parent = nullptr);
    SyncableObject(const SyncableObject& other, QObject* parent = nullptr);
    ~SyncableObject() override;

    /**
     * Stores the object's state in a QVariantMap.
     *
     * The returned map contains the values of all properties, indexed by property name.
     *
     * @sa fromVariantMap
     *
     * @returns The object's state in a QVariantMap
     */
    QVariantMap toVariantMap();

    /**
     * Initializes the object's state from a given QVariantMap.
     *
     * Writable properties are updated from the values stored in the given map.
     *
     * @note Properties not contained in the map are left alone; they're not reset to default values.
     *
     * @sa toVariantMap
     *
     * @param properties Map containing property values, indexed by property name
     */
    void fromVariantMap(const QVariantMap& properties);

    virtual bool isInitialized() const;

    virtual const QMetaObject* syncMetaObject() const = 0;

    /**
     * Returns the proxy mode of the SignalProxy this instance is synchronized with.
     *
     * @returns The SignalProxy's proxy mode if synchronized, SignalProxy::ProxyMode::Unknown otherwise
     */
    SignalProxy::ProxyMode proxyMode() const;

    inline void setAllowClientUpdates(bool allow) { _allowClientUpdates = allow; }
    inline bool allowClientUpdates() const { return _allowClientUpdates; }

    /**
     * Derives a property's setter name from the given NOTIFY signal.
     *
     * Unfortunately, Qt provides no way to get the name of the property's write method; only the NOTIFY signal,
     * if declared, is exposed with its meta information.
     * Thus, we have to rely on a naming convention and heuristics to derive the setter name from that signal:
     *
     * - If the signal name starts with "sync", the remainder of the name is used as the setter, with the first letter
     *   converted to lowercase; e.g. "syncAnySlot" becomes "anySlot". This allows for defining custom setter names.
     *
     * - If the signal name ends with "Set" or "Changed", then that suffix is removed and "set" prefixed, so "myFooChanged"
     *   becomes "setMyFoo". This covers a vast majority of existing setters and their notify signals, and is the recommended
     *   naming scheme when introducing new properties.
     *
     * If the given signal isn't valid, or if it does not adhere to the naming scheme, an empty byte array is returned.
     *
     * @param notifySignal A property's NOTIFY signal
     * @returns The property's setter name derived from the signal name if it adheres to the naming scheme, empty byte array otherwise
     */
    static QByteArray propertySetter(const QMetaMethod& notifySignal);

    /**
     * Checks if a property corresponds to the given setter name, and if successful, sets its value.
     *
     * Matches the given setterName against the setters derived from the supported properties (as determined by @a propertySetter),
     * and if a match is found, sets the property's value by calling QMetaProperty::write(), which is more efficient than
     * invoking the setter dynamically. This is mainly useful for syncing properties from a SyncMessage in a more efficient way.
     *
     * The supported properties are cached, so subsequent calls of this method are fast.
     *
     * @param setterName The setter name
     * @param value      The value to set
     * @returns true, if a property was found and the value could be set successfully
     */
    bool syncProperty(const QByteArray& setterName, const QVariant& value);

    /**
     * @returns This object's sync method map
     */
    virtual SyncMethodMap syncMethodMap() const = 0;

    /**
     * Invokes the given sync method on this instance with the given arguments.
     *
     * If a non-void request method is called, and a matching receive method exists that can accept the return value,
     * the returned SyncMessage is filled with the data appropriate for calling the receive method on the requesting
     * peer. Otherwise, the message is empty.
     *
     * @param syncMethodName Name of the sync method to be invoked
     * @param params         Arguments for the sync method call
     * @returns A SyncMessage (non-empty if a receive method should be invoked) in case of success, boost::none otherwise
     */
    boost::optional<Protocol::SyncMessage> invokeSyncMethod(const QByteArray& syncMethodName, const QVariantList& params);

public slots:
    virtual void setInitialized();
    void requestUpdate(const QVariantMap& properties);
    virtual void update(const QVariantMap& properties);

signals:
    void initDone();
    void updatedRemotely();
    void updated();

protected:
    SyncableObject& operator=(const SyncableObject& other);
    void sync_call__(SignalProxy::ProxyMode modeType, const char* funcname, ...) const;

    /**
     * Finds the index of the given method name in the given method name list at compile time.
     *
     * @param syncMethod      Sync method name to be queried
     * @param declaredMethods Array containing all declared method names
     * @returns The index of syncMethod in the declaredMethods array, or -1 if it is not found
     */
    template<std::size_t N>
    static constexpr int syncMethodIndex(const char* syncMethod, const SyncMethodNames<N>& declaredMethods);

    /**
     * Builds a compile-time array containing the given method names.
     *
     * @param names The method names (as const char*)
     * @returns A SyncMethodNames array initialized with the given names
     */
    template<typename ...Names>
    static constexpr SyncMethodNames<sizeof...(Names)> buildSyncMethodNames(Names... names);

    /**
     * Builds a map that, for every sync method given, holds a call wrapper for invoking the method.
     *
     * Forwards to an overload that takes an index sequence for unpacking the tuple.
     *
     * @param methodNames Sync method names
     * @param methodPtrs  Sync method pointers
     * @returns A map holding sync method call wrappers indexed by their name
     */
    template<typename ...Ptrs, std::size_t N>
    static SyncMethodMap buildSyncMethodMap(const SyncMethodNames<N>& methodNames, const std::tuple<Ptrs...>& methodPtrs);

private:
    void synchronize(QPointer<SignalProxy> proxy);
    void stopSynchronize(QPointer<SignalProxy> proxy);

    int propertyIndex(const QString& propertyName) const;

    /**
     * @overload of buildSyncMethodMap that takes an index sequence for unpacking the methodPtr tuple.
     */
    template<typename ...Ptrs, std::size_t N, std::size_t ...Is>
    static SyncMethodMap buildSyncMethodMap(const SyncMethodNames<N>& methodNames,
                                            const std::tuple<Ptrs...>& methodPtrs,
                                            std::index_sequence<Is...>);

    /**
     * Determines the number of arguments of RecvMethod's signature that match the given return type and potentially argument types.
     *
     * Used for checking if a given potential receive method can be invoked for a request method that takes ReqArgs and returns ReqRet.
     * Given the way legacy SyncableObjects (protocol version 10) worked in Quassel, a receive method either takes just the return value,
     * or expects to be invoked with all arguments used for calling the request method, followed by the return value. Consequently, the
     * value returned by matchingArgCount() is either equal to the number of arguments in RecvMethod's signature, or -1 if the signature
     * does not match.
     *
     * @tparam RecvMethod Candidate receive method
     * @tparam ReqRet     Request method's return type
     * @tparam ReqArgs    Request method's argument types
     * @returns The number of matching arguments, i.e. the number of RecvMethod's arguments, or -1 if the method signatures don't match
     */
    template<typename RecvMethod, typename ReqRet, typename ...ReqArgs>
    static constexpr int matchingArgCount();

    /**
     * Builds a call wrapper for the given sync method.
     *
     * The callable returned by this function can be invoked with a SyncableObject pointer and a QVariantList containing the arguments,
     * and returns an optional holding a SyncMessage (or boost::none, if the call was not successful). If the returned SyncMessage is
     * both valid and filled with data, it should be sent back to the requesting peer to invoke a corresponding receive method there.
     *
     * The call wrapper also checks for the correct SignalProxy mode; normal sync methods can only be invoked on client-side SyncableObject
     * instances, while request methods can only be invoked core-side.
     *
     * @param methodName  The sync method's name
     * @param syncMethod  Member function pointer to the sync method
     * @param methodNames Reference to the full list of declared sync method names
     * @returns A call wrapper for the given sync method
     */
    template<typename ...Ptrs, typename R, typename C, typename ...Args, std::size_t N>
    static SyncMethodWrapper buildSyncMethodWrapper(const QByteArray& methodName, R(C::*syncMethod)(Args...),
                                                    const SyncMethodNames<N>& methodNames);

private:
    QString _objectName;
    bool _initialized{false};
    bool _allowClientUpdates{false};

    QPointer<SignalProxy> _signalProxy;

    friend class SignalProxy;
};

// ---- Template method definitions -------------------------------------------------------------------------------------------------------

template<std::size_t N>
constexpr int SyncableObject::syncMethodIndex(const char* syncMethod, const SyncMethodNames<N>& declaredMethods)
{
    // We can't use an algorithm or anything fancy in a constexpr method
    for (std::size_t i = 0; i < declaredMethods.size(); ++i) {
        if (rawStringsAreEqual(syncMethod, declaredMethods[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

template<typename ...Names>
constexpr SyncableObject::SyncMethodNames<sizeof...(Names)> SyncableObject::buildSyncMethodNames(Names... names)
{
    return {{names...}};
}

template<typename ...Ptrs, std::size_t N>
SyncableObject::SyncMethodMap SyncableObject::buildSyncMethodMap(const SyncMethodNames<N>& methodNames,
                                                                 const std::tuple<Ptrs...>& methodPtrs)
{
    static_assert(sizeof...(Ptrs) == N, "");
    return buildSyncMethodMap(methodNames, methodPtrs, std::make_index_sequence<N>{});
}

template<typename ...Ptrs, std::size_t N, std::size_t ...Is>
SyncableObject::SyncMethodMap SyncableObject::buildSyncMethodMap(const SyncMethodNames<N>& methodNames,
                                                                 const std::tuple<Ptrs...>& methodPtrs,
                                                                 std::index_sequence<Is...>)
{
    return {std::make_pair(methodNames[Is], buildSyncMethodWrapper<Ptrs...>(methodNames[Is], std::get<Is>(methodPtrs), methodNames))...};
}

template<typename RecvMethod, typename ReqRet, typename ...ReqArgs>
constexpr int SyncableObject::matchingArgCount()
{
    using RecvArgs = traits::decay_t<traits::args_t<RecvMethod>>;
    if (std::tuple_size<RecvArgs>::value == 1) {
        return std::is_same<std::tuple_element_t<0, RecvArgs>, std::decay_t<ReqRet>>::value ? 1 : -1;
    }

    return std::is_same<RecvArgs, traits::decay_t<std::tuple<ReqArgs..., ReqRet>>>::value
               ? static_cast<int>(std::tuple_size<RecvArgs>::value)
               : -1;
}

template<typename ...Ptrs, typename R, typename C, typename ...Args, std::size_t N>
SyncableObject::SyncMethodWrapper SyncableObject::buildSyncMethodWrapper(const QByteArray& methodName,
                                                                         R(C::*syncMethod)(Args...),
                                                                         const SyncMethodNames<N>& methodNames)
{
    // Request methods are intended to be called core-side, and may return a value
    if (methodName.startsWith("request")) {
        QByteArray recvMethodName;
        int matchingArgs{-1};

        // Non-void request method, so check if we have a matching receive method that would accept the return value
        if (!std::is_same<R, void>::value) {
            recvMethodName = "receive" + methodName.mid(7);
            auto recvMethodIdx = syncMethodIndex(recvMethodName.constData(), methodNames);
            if (recvMethodIdx >= 0) {
                matchingArgs = std::array<int, sizeof...(Ptrs)>{{matchingArgCount<Ptrs, R, Args...>()...}}[recvMethodIdx];
            }
            if (matchingArgs < 1) {
                qWarning() << "No matching receive method for non-void request method" << methodName;
            }
        }

        return [syncMethod, methodName, recvMethodName = std::move(recvMethodName), matchingArgs](SyncableObject* obj, const QVariantList& params)
                   -> boost::optional<Protocol::SyncMessage>
        {
            if (obj->proxyMode() != SignalProxy::ProxyMode::Server) {
                qWarning().noquote().nospace() << "Request method "
                                               << obj->syncMetaObject()->className() << "::" << methodName
                                               << " cannot be invoked on the client side";
                return boost::none;
            }
            auto retVal = invokeWithArgsList(qobject_cast<C*>(obj), syncMethod, params);
            if (!retVal) {
                return boost::none;
            }
            if (matchingArgs > 0) {
                // We have a matching receive method
                if (retVal->isValid()) {
                    QVariantList retParams;
                    if (matchingArgs > 1) {
                        // If the receive method accepts more than one parameter, it expects the input params to be sent back
                        retParams = params;
                    }
                    retParams << *retVal;
                    return Protocol::SyncMessage{obj->syncMetaObject()->className(), obj->objectName(), recvMethodName, std::move(retParams)};
                }
                else {
                    qWarning().noquote().nospace() << "Request method "
                                                   << obj->syncMetaObject()->className() << "::" << methodName
                                                   << " returned an invalid value";
                }
            }
            return Protocol::SyncMessage{};
        };
    }

    // Not a request method, simply invoke (only possible on the client side)
    return [syncMethod, methodName](SyncableObject* obj, const QVariantList& params) -> boost::optional<Protocol::SyncMessage> {
        if (obj->proxyMode() != SignalProxy::ProxyMode::Client) {
            qWarning().noquote().nospace() << "Sync method "
                                           << obj->syncMetaObject()->className() << "::" << methodName
                                           << " cannot be invoked on the core side";
            return boost::none;
        }
        return invokeWithArgsList(qobject_cast<C*>(obj), syncMethod, params) ? boost::make_optional(Protocol::SyncMessage{}) : boost::none;
    };
}
