#ifndef __forte__h
#define __forte__h

#include <cstdarg>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

//using namespace std;
//using namespace __gnu_cxx;
namespace Forte {};
//using namespace Forte;

#include "Object.h"

#include "AutoFD.h"
#include "AutoMutex.h"
#include "Base64.h"
#include "CheckedDouble.h"
#include "CheckedInt32.h"
#include "CheckedStringEnum.h"
#include "Clock.h"
#include "CheckedValueStore.h"
#include "Collector.h"
#include "Condition.h"
#include "Context.h"
#include "ContextPredicate.h"

#ifndef FORTE_NO_CURL
#include "Curl.h"
#endif

#ifndef FORTE_NO_DB
#include "DbAutoConnection.h"
#include "DbAutoTrans.h"
#include "DbConnection.h"
#include "DbConnectionPool.h"
#include "DbException.h"
#include "DbResult.h"
#include "DbRow.h"
#include "DbUtil.h"
#ifndef FORTE_NO_MYSQL
#include "DbMyConnection.h"
#include "DbMyResult.h"
#endif
#ifdef FORTE_WITH_POSTGRESQL
#include "DbPgConnection.h"
#include "DbPgResult.h"
#endif
#ifndef FORTE_NO_SQLITE
#include "DbLiteConnection.h"
#include "DbLiteResult.h"
#endif
#endif // FORTE_NO_DB

#include "DaemonUtil.h"
#include "Dispatcher.h"
#include "Event.h"
#include "EventQueue.h"
#include "Exception.h"
#include "ExpDecayingAvg.h"
#include "FileSystem.h"
#include "FileSystemUtil.h"
#include "Foreach.h"
#include "FString.h"
#include "FTrace.h"
#include "LogManager.h"
#include "LogTimer.h"
#include "FMD5.h"
#include "Murmur.h"
#include "OnDemandDispatcher.h"
#ifndef FORTE_NO_OPENSSL
#include "OpenSSLInitializer.h"
#endif
#include "ProcFileSystem.h"
#include "ProcRunner.h"
#include "PidFile.h"
#include "Random.h"
#include "ReceiverThread.h"
#include "RequestHandler.h"
#include "RWLock.h"
#include "Semaphore.h"
#include "ServiceConfig.h"
#include "ServerMain.h"
#include "StateAction.h"
#include "StateMachine.h"
#include "Thread.h"
#include "ThreadCondition.h"
#include "ThreadKey.h"
#include "ThreadPoolDispatcher.h"
#include "Types.h"
#include "Util.h"
#include "UrlString.h"
#include "VersionManager.h"

#ifndef FORTE_NO_XML
#include "XMLBlob.h"
#include "XMLDoc.h"
#include "XMLNode.h"
#include "XMLTextNode.h"
#endif

#endif
