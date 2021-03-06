#include "Timer.hpp"
#include "StringConversion.hpp"
#include "SignalThread.hpp"

#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

namespace SynerEdge
{

struct Timer::InnerData : private boost::noncopyable
{
	InnerData(unsigned long millis_, bool repeat_)
	: millis(millis_), repeat(repeat_), id(0)
	{}

	unsigned long millis;
	bool repeat;
	timer_t id;
	struct sigevent sigEvent;

private:
	// non-copyable semantics
	InnerData(const InnerData &);
	InnerData &operator=(const InnerData &equal);
};

Timer::Timer(unsigned long millis, bool repeat)
: innerData(*(new InnerData(millis, repeat))), timerExpired(this)
{
	innerData.sigEvent.sigev_value.sival_ptr = 
		reinterpret_cast<void *>(&timerExpired);
	innerData.sigEvent.sigev_signo = SIGALRM;
	innerData.sigEvent.sigev_notify = SIGEV_SIGNAL;

	if (timer_create(CLOCK_REALTIME, 
	                 &(innerData.sigEvent), 
	                 &(innerData.id)) == -1)
	{
		throw TimerException(StringConversion::syserr());
	}
}

Timer::~Timer()
{
	timer_delete(innerData.id);
}

void Timer::start()
{
	SignalThread::instance(SIGALRM, true);

	struct itimerspec timer_val;

	if (innerData.repeat)
	{
		timer_val.it_interval.tv_sec = 
			(innerData.millis / 1000);
		timer_val.it_interval.tv_nsec = 
			(innerData.millis % 1000) * 1000;
	}
	else
	{
		timer_val.it_interval.tv_sec = 0;
		timer_val.it_interval.tv_nsec = 0;
	}

	timer_val.it_value.tv_sec = (innerData.millis / 1000);
	timer_val.it_value.tv_nsec = (innerData.millis % 1000) * 1000;

	if (timer_settime(innerData.id, 0, &timer_val, 0) == -1)
	{
		throw TimerException(StringConversion::syserr());
	}
}

void Timer::reset(unsigned long millis, bool repeat)
{
	innerData.millis = millis;
	innerData.repeat = repeat;

	start();
}

void Timer::sleep(unsigned long millis)
{
	struct timespec itm, otm;

	itm.tv_sec = (millis / 1000);
	itm.tv_nsec = (millis % 1000) * 1000;

	while (1)
	{
		if (nanosleep(&itm, &otm) != 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else 
			{
				throw new TimerException(
					StringConversion::syserr());
			}
		}
		break;
	}
}

}

