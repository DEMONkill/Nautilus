#ifndef SygObserver_hpp
#define SygObserver_hpp

#include <list>
#include <iostream>
#include <memory>
#include "boost/thread.hpp"
#include "boost/thread/mutex.hpp"

namespace SynerEdge
{

// BaseEventArgs
//
// not really needed, just provides a good base class
// for EventArgs.
class BaseEventArgs
{
public:
    BaseEventArgs() {}
    virtual ~BaseEventArgs() {}
};

// Observable
//
// nothing interesting, just provides us a base class
// for anything that will be observed.  This allows
// dynamic_casting in the delegate's callback method.
class Observable
{
public:
        Observable();
        virtual ~Observable();
};

// BaseObserverDelegate
// 
// This is a base class for all ObserverDelegates.  The
// template paramters is for the EventArgs class.  By
// convention, that class should be extended from the
// BaseEventArgs class (but this is not required).
//
// Note that the functions in this class are largely
// independent of the template parameter.  Only the
// functor operator() makes use of it, assuring that
// the "passthru" parameter to the function matches
// what you expect.  The virtual function isEqual
// is implemented in the derived template (below) and 
// it is used to determine equality of two delegates.
template<class EventArgs> class BaseObserverDelegate
{
public:
    BaseObserverDelegate() {}
    virtual void operator() (Observable *obs, const EventArgs &args) = 0;
    virtual BaseObserverDelegate *clone() = 0;
    bool operator==(const BaseObserverDelegate &equal)
    {
	return isEqual(equal);
    }
    virtual bool isEqual(const BaseObserverDelegate &equal) = 0;
};

// ObserverDelegate
//
// This class is essentially a fancy functor for an object and
// a method pointer on the object.  The method pointer should
// have the following signature: 
//
//     void (T::*func) (Observer *obs, const EventArgs &args)
//
// The first template parameter is to indicate the class type
// of the object to receive the callback.  The second template
// parameter is to define the EventArgs class that gets passed

// along to the delegate when the event is signaled.
//
// The delegate can be called by the use of operator() on the
// class.  Delegates are always new'd when passed into events,
// so they must be compared with operator== on the class. This
// function makes that the delegates are actually of the same
// type, and makes sure the object and the method pointer of the
// two delegates are the same.
//
// ObserverDelegates are generally expected to be used in the
// following way:
// 
// -- instantiate an instance of your callback class: CallbackClass cc;
// -- your server object should have an ObservableEvent that is a public
// -- member (called 'event' in this example).
//
// --subscribe to the event
// server.event += new ObserverDelegate<CallbackClass, MyEventArgs>
//                          (cc, &CallbackClass::func);
//
// --unsubscribe to the event
// server.event -= new ObserverDelegate<CallbackClass, MyEventArgs>
//			    (cc, &CallbackCall::func);
template<class T, class EventArgs> class ObserverDelegate : public BaseObserverDelegate<EventArgs>
{
public:
    typedef void (T::*MemberPtr)(Observable *obs, const EventArgs &args);

    ObserverDelegate() : mptr(0), obj(0) {}
    ObserverDelegate(const ObserverDelegate &copy) : mptr(copy.mptr), obj(copy.obj) {}
    ObserverDelegate(T &obj_, MemberPtr mptr_) : obj(&obj_), mptr(mptr_) {}
    virtual ~ObserverDelegate() {}
    virtual void operator() (Observable *obs, const EventArgs &args) 
    { 
	(obj->*mptr)(obs, args); 
    }
    virtual BaseObserverDelegate<EventArgs> *clone()
    {
	return new ObserverDelegate<T, EventArgs>(*this);
    }
    virtual bool isEqual(const BaseObserverDelegate<EventArgs> &equal)
    {
	bool result = false;
	const ObserverDelegate *test = dynamic_cast<const ObserverDelegate *>(&equal);
	if (test != 0)
	{
		result = ((obj == test->obj) && (mptr == test->mptr));
	}
	return result;
    }

    T *obj;
    MemberPtr mptr;
};

// ObservableEvent
//
// This class is constructed with an Observable object.  This ensures
// that the event (when signalled) can pass a base pointer for the
// Observed object along with the EventArgs to the class.
//
// The template parameter is only provided to describe the EventArgs
// that should be passed along with the event signalling (operator())
// functor.
//
// The ObservableEvent is expected to be used in the following
// way:
//
// class Observed : public Observable
// {
// public:
//	Observed : myEvent(this) {}
//	ObservableEvent<MyEventArgs> myEvent;
//
//      void fireMyEvent(const MyEventArgs &args)
//	{
//		myEvent(args);	
//	}
// };
template<class EventArgs> class ObservableEvent
{
public:
    // Pass in the Observed pointed in the constructor of the
    // event.  This is normally the this pointer of the class
    // that is containing the event.  We also initialize the 
    // array to a max of 10 items here.  This can grow if more
    // delegates subscribe.
    ObservableEvent(Observable *obs_) : obs(obs_)
    {
	max_count = 10;
	count = 0;
	mylist = new BaseObserverDelegate<EventArgs>*[max_count];
	for (int i = 0; i < max_count; i++) mylist[i] = 0;
    }

    // You must clean up all subscribed delegates.  That means
    // deleting the ones still in the array, and deleting the
    // array itself.
    virtual ~ObservableEvent() 
    {
	for (int i = 0; i < count; i++) delete mylist[i];
	delete[] mylist;
    }

    // Subscribe a delegate.  Remember the delegate is always new'd
    // when passed into operator+= and operator-=.  Extend list
    // by 10 if you have already surpassed the count of the array.
    ObservableEvent &operator+=(BaseObserverDelegate<EventArgs> *subscriber)
    {
	boost::mutex::scoped_lock lk(mtx, true);

	if (count >= max_count)
	{
		max_count = max_count + 10;
		BaseObserverDelegate<EventArgs>** newlist = 
			new BaseObserverDelegate<EventArgs>*[max_count];
		for (int k = 0; k < max_count; k++) newlist[k] = 0;
		int j = 0;
		for (int i = 0; i < count; i++)
		{
			newlist[j++] = mylist[i];
		}
		delete[] mylist;
		mylist = newlist;
	}
	mylist[count++] = subscriber;
        return *this;
    }

    // Unsubscribe a delegate.  Remember the delegate is always new'd
    // when passed to operator+= or operator-=, so you cannot compare
    // pointers, you must rely on operator== on the BaseObserverDelegate
    // to tell you which item in the list matches the subscriber passed
    // in.  You must also delete both the removed delegate, and the
    // delegate passed in or you will likely have a leak.
    ObservableEvent &operator-=(BaseObserverDelegate<EventArgs> *subscriber)
    {
	boost::mutex::scoped_lock lk(mtx, true);

	for (int i = 0; i < count; i++)
	{ 
		if (*mylist[i] == *subscriber)
		{
			delete mylist[i];
			--count;
			for (int j = i; j < count; j++)
			{
				mylist[j] = mylist[j+1];
			}
			mylist[count] = 0;
			break;
		}
	}

	delete subscriber;
        return *this;
    }

    // Fire event and notify all subscribers
    void operator() (const EventArgs &args)
    {
	boost::mutex::scoped_lock lk(mtx, true);

	for (int i = 0; i < count; i++)
	{
		(*mylist[i])(obs, args);
	}
    }

private:
   Observable *obs;

   // normally I would just embed a std::list template
   // to hold these, but there is some problem in g++
   // in doing that - so I punted, and am managing my
   // own list.
   BaseObserverDelegate<EventArgs> **mylist;
   size_t max_count;
   size_t count;

   boost::mutex mtx;
    
};


}; // namespace

#endif
