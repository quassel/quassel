#include "backgroundtaskhandler.h"

#include "core.h"
#include "coresession.h"

BackgroundTaskHandler::BackgroundTaskHandler(CoreSession* coreSession)
    : QObject(nullptr),
    _coreSession(coreSession) {
    _workerThread.start();
    moveToThread(&_workerThread);
}

BackgroundTaskHandler::~BackgroundTaskHandler()
{
    _workerThread.quit();
    _workerThread.wait();
}

void BackgroundTaskHandler::deleteBuffer(BufferId bufferId) {
    QThread::sleep(10);
    Core::removeBuffer(_coreSession->user(), bufferId);
    QThread::sleep(10);
    emit onBufferDeleted(bufferId);
}