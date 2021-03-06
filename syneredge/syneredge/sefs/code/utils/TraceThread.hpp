#ifndef SynerEdge_TraceThread_hpp
#define SynerEdge_TraceThread_hpp

#include "SemaQueue.hpp"
#include "Observer.hpp"
#include "SynerEdge.hpp"
#include "StringConversion.hpp"

namespace SynerEdge
{

#define TRACE_SETUP(subsys) TraceData $traceDataStruct$((subsys))

#define TRACE(level, mesg) $traceDataStruct$.traceLevel = (level); \
$traceDataStruct$.msg = (mesg); \
$traceDataStruct$.filename = StringConversion::toUTF16(__FILE__); \
$traceDataStruct$.lineNumber = __LINE__; \
$traceDataStruct$.module = StringConversion::toUTF16(__FUNCTION__); \
$traceDataStruct$.threadId = static_cast<syg_ulong_ptr>($traceDataStruct$.threadId); \
TraceThread::instance()->trace($traceDataStruct$)

struct TraceData
{
	enum TraceLevel { info, warning, exception, assert };

	TraceData(const std::wstring &subsystem);

	TraceData(const std::wstring &subsystem, 
	          enum TraceLevel traceLevel, 
		  const std::wstring &msg,
		  const std::string &filename,
	  	  long lineNumber,
		  const std::string &module,
		  syg_ulong_ptr threadId);

	std::wstring subsystem;
	enum TraceLevel traceLevel;
	std::wstring msg;
	syg_ulong_ptr threadId;
	std::wstring filename;
	long lineNumber;
	std::wstring module;
};

std::wostream &operator<<(std::wostream &out, const TraceData &td);

class TraceThread : private boost::noncopyable, public Observable
{
public:
	class BoostThread
	{
	public:
		BoostThread(TraceThread *instance);
		void operator()();
	private:
		TraceThread *_instance;
	};

	~TraceThread();
	static TraceThread *instance();

	ObservableEvent<TraceData> receivedTrace;	
	ObservableEvent<TraceData> receivedLog;	

	void requestStop();

	void trace(TraceData &td);

	static void haltIfRunning();

private:
	SemaQueue<TraceData> traceQue;
	bool stopRequested;
	boost::thread *tracingThread;

	// static class variables - instance and sentry
	static boost::once_flag _sentry;
	static boost::mutex _mtx;
	static TraceThread *_instance;

	// singleton semantics
	TraceThread();
	static void createInstance();

	// noncopyable semantics
	TraceThread(const TraceThread &);
	TraceThread &operator=(const TraceThread &);
};

}

#endif
