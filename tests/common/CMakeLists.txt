quassel_add_test(ExpressionMatchTest)

quassel_add_test(InvokeTest)

quassel_add_test(SignalProxyTest
    LIBRARIES
        Quassel::Test::Util
)
