quassel_add_test(ExpressionMatchTest)

quassel_add_test(FuncHelpersTest)

quassel_add_test(IrcDecoderTest)

quassel_add_test(IrcEncoderTest)

quassel_add_test(SignalProxyTest
    LIBRARIES
        Quassel::Test::Util
)

quassel_add_test(UtilTest)
