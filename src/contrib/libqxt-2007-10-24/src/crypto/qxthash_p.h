#include "md5.h"
#include "md4.h"
class QxtHashPrivate : public QxtPrivate<QxtHash>
{
public:

    QxtHash::Algorithm algo;

    MD5Context md5ctx;
    md4_context md4ctx;
};

