#pragma once

#include "syncableobject.h"
#include "types.h"

class CoreSession;
class BackgroundTaskHandler : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundTaskHandler(CoreSession* coreSession);
    ~BackgroundTaskHandler() override;
public slots:
    void deleteBuffer(BufferId bufferId);
signals:
    void onBufferDeleted(BufferId);
private:
    CoreSession* _coreSession;
    QThread _workerThread;
};