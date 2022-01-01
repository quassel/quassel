/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include "mockedpeer.h"

#include <boost/any.hpp>

using namespace ::testing;

namespace test {

// ---- Protocol message wrapper -----------------------------------------------------------------------------------------------------------

struct ProtocolMessage
{
    // Supported protocol message types. If extended, the various visitors in this file need to be extended too.
    boost::variant<
        Protocol::SyncMessage,
        Protocol::RpcCall,
        Protocol::InitRequest,
        Protocol::InitData
    > message;
};

void PrintTo(const ProtocolMessage& msg, std::ostream* os)
{
    struct PrintToVisitor : public boost::static_visitor<void>
    {
        PrintToVisitor(std::ostream* os)
            : _os{os}
        {}

        void operator()(const Protocol::SyncMessage& syncMessage) const
        {
            *_os << "SyncMessage{className = " << PrintToString(syncMessage.className)
                 << ", objectName = " << PrintToString(syncMessage.objectName)
                 << ", slotName = " << PrintToString(syncMessage.slotName)
                 << ", params = " << PrintToString(syncMessage.params)
                 << "}";
        }

        void operator()(const Protocol::RpcCall& rpcCall) const
        {
            *_os << "RpcCall{signalName = " << PrintToString(rpcCall.signalName)
                 << ", params = " << PrintToString(rpcCall.params)
                 << "}";
        }

        void operator()(const Protocol::InitRequest& initRequest) const
        {
            *_os << "InitRequest{className = " << PrintToString(initRequest.className)
                 << ", objectName = " << PrintToString(initRequest.objectName)
                 << "}";
        }

        void operator()(const Protocol::InitData& initData) const
        {
            *_os << "InitData{className = " << PrintToString(initData.className)
                 << ", objectName = " << PrintToString(initData.objectName)
                 << ", initData = " << PrintToString(initData.initData)
                 << "}";
        }

    private:
        std::ostream* _os;
    };

    boost::apply_visitor(PrintToVisitor{os}, msg.message);
}

// ---- MockedPeer -------------------------------------------------------------------------------------------------------------------------

MockedPeer::MockedPeer(QObject* parent)
    : InternalPeer(parent)
{
    // Default behavior will just delegate to InternalPeer (through the mocked Dispatches() method)
    ON_CALL(*this, Dispatches(_)).WillByDefault(Invoke(this, &MockedPeer::dispatchInternal));
}

MockedPeer::~MockedPeer() = default;

void MockedPeer::dispatch(const Protocol::SyncMessage& msg)
{
    Dispatches({msg});
}

void MockedPeer::dispatch(const Protocol::RpcCall& msg)
{
    Dispatches({msg});
}

void MockedPeer::dispatch(const Protocol::InitRequest& msg)
{
    Dispatches({msg});
}

void MockedPeer::dispatch(const Protocol::InitData& msg)
{
    Dispatches({msg});
}

// Unwraps the type before calling the correct overload of realDispatch()
struct DispatchVisitor : public boost::static_visitor<void>
{
    DispatchVisitor(MockedPeer* mock)
        : _mock{mock}
    {}

    template<typename T>
    void operator()(const T& message) const
    {
        _mock->realDispatch(message);
    }

    MockedPeer* _mock;
};

void MockedPeer::dispatchInternal(const ProtocolMessage& message)
{
    boost::apply_visitor(DispatchVisitor{this}, message.message);
}

template<typename T>
void MockedPeer::realDispatch(const T& message)
{
    InternalPeer::dispatch(message);
}

// ---- Expectations and matchers ----------------------------------------------------------------------------------------------------------

namespace {

struct SyncMessageExpectation
{
    Matcher<QByteArray> className;
    Matcher<QString> objectName;
    Matcher<QByteArray> slotName;
    Matcher<QVariantList> params;
};

struct RpcCallExpectation
{
    Matcher<QByteArray> signalName;
    Matcher<QVariantList> params;
};

struct InitRequestExpectation
{
    Matcher<QByteArray> className;
    Matcher<QString> objectName;
};

struct InitDataExpectation
{
    Matcher<QByteArray> className;
    Matcher<QString> objectName;
    Matcher<QVariantMap> initData;
};

/**
 * Generic matcher for protocol messages.
 *
 * @note The matcher, maybe somewhat surprisingly, always matches; it does, however, add test failures if expectations fail.
 *       This makes for a much better readable output when used in EXPECT_CALL chains, while still letting test cases fail
 *       as expected.
 */
class ProtocolMessageMatcher : public MatcherInterface<const ProtocolMessage&>
{
public:
    template<typename T>
    ProtocolMessageMatcher(T expectation)
        : _expectation{std::move(expectation)}
    {}

    /**
     * Visitor used for matching a particular type of protocol message.
     *
     * Each supported type requires a corresponding overload for the call operator.
     */
    struct MatchVisitor : public boost::static_visitor<bool>
    {
        MatchVisitor(const boost::any& expectation)
            : _expectation{expectation}
        {}

        bool operator()(const Protocol::SyncMessage& syncMessage) const
        {
            auto e = boost::any_cast<SyncMessageExpectation>(&_expectation);
            if (!e) {
                ADD_FAILURE() << "Did not expect a SyncMessage!";
                return true;
            }
            EXPECT_THAT(syncMessage.className, e->className);
            EXPECT_THAT(syncMessage.objectName, e->objectName);
            EXPECT_THAT(syncMessage.slotName, e->slotName);
            EXPECT_THAT(syncMessage.params, e->params);
            return true;
        }

        bool operator()(const Protocol::RpcCall& rpcCall) const
        {
            auto e = boost::any_cast<RpcCallExpectation>(&_expectation);
            if (!e) {
                ADD_FAILURE() << "Did not expect an RpcCall!";
                return true;
            }
            EXPECT_THAT(rpcCall.signalName, e->signalName);
            EXPECT_THAT(rpcCall.params, e->params);
            return true;
        }

        bool operator()(const Protocol::InitRequest& initRequest) const
        {
            auto e = boost::any_cast<InitRequestExpectation>(&_expectation);
            if (!e) {
                ADD_FAILURE() << "Did not expect an InitRequest!";
                return true;
            }
            EXPECT_THAT(initRequest.className, e->className);
            EXPECT_THAT(initRequest.objectName, e->objectName);
            return true;
        }

        bool operator()(const Protocol::InitData& initData) const
        {
            auto e = boost::any_cast<InitDataExpectation>(&_expectation);
            if (!e) {
                ADD_FAILURE() << "Did not expect InitData!";
                return true;
            }
            EXPECT_THAT(initData.className, e->className);
            EXPECT_THAT(initData.objectName, e->objectName);
            EXPECT_THAT(initData.initData, e->initData);
            return true;
        }

    private:
        const boost::any& _expectation;
    };

    bool MatchAndExplain(const ProtocolMessage& protoMsg, MatchResultListener*) const override
    {
        return boost::apply_visitor(MatchVisitor{_expectation}, protoMsg.message);
    }

    void DescribeTo(std::ostream* os) const override
    {
        // This should never be actually called because we always match (but fail sub-expectations if appropriate)
        *os << "Matcher for protocol messages";
    }

private:
    boost::any _expectation;
};

}  // anon

// Create matcher instances

Matcher<const ProtocolMessage&> SyncMessage(Matcher<QByteArray> className, Matcher<QString> objectName, Matcher<QByteArray> slotName, Matcher<QVariantList> params)
{
    return MakeMatcher(new ProtocolMessageMatcher{SyncMessageExpectation{std::move(className), std::move(objectName), std::move(slotName), std::move(params)}});
}

Matcher<const ProtocolMessage&> RpcCall(Matcher<QByteArray> signalName, Matcher<QVariantList> params)
{
    return MakeMatcher(new ProtocolMessageMatcher{RpcCallExpectation{std::move(signalName), std::move(params)}});
}

Matcher<const ProtocolMessage&> InitRequest(Matcher<QByteArray> className, Matcher<QString> objectName)
{
    return MakeMatcher(new ProtocolMessageMatcher{InitRequestExpectation{std::move(className), std::move(objectName)}});
}

Matcher<const ProtocolMessage&> InitData(Matcher<QByteArray> className, Matcher<QString> objectName, Matcher<QVariantMap> initData)
{
    return MakeMatcher(new ProtocolMessageMatcher{InitDataExpectation{std::move(className), std::move(objectName), std::move(initData)}});
}

}  // namespace test
