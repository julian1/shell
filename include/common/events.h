
#pragma once

// whether something gets wraped and composed - by a following system 
// or else injected with references doesn't matter. they both create
// isolated composable systems with good event properties.  

// we can do cancellable events as well.

// this is a version without bind or function
// it relies on the event handler to cast to the 
// correct type

#include <vector>
#include <algorithm>
#include <iostream>
#include <cstring>
//#include <boost/bind.hpp>
//#include <boost/function.hpp>

struct INotify;


// change name notify() to emit()

struct IObject
{
	// events
	virtual void register_( INotify & ) = 0;
	virtual void unregister( INotify & ) = 0;

	// we have to have something we can cast from ...
	// this means evertything that can emit events has to be
	// derived from this.
	//virtual ~IObject() { } 
};

// The thing is to perform any casting ... - for the receiver. 
// the receiver knows what type of object it expects. 

// the downcast has to be done however. because the emitdowncast, 

// would it be possible to wrap the object somehow? 


struct Event
{
	Event( IObject & object, const char *msg )
		: object( object),
		msg( msg),
		payload( 0),
		cancel( false)
	{ } 
	// perhaps an enum
	IObject & object;
	const char *msg;
	void *payload;
	bool cancel;
};


struct INotify
{
	virtual void notify( const Event &e ) = 0;

	// actually we can't manage the memory from an interface
	// unless we newed it.
};

struct Events
{
	std::vector< INotify *> listeners;  

	void register_( INotify & l) 
	{
		listeners.push_back( &l);
	}
	void unregister( INotify & l) 
	{
		listeners.erase(	
			std::remove( listeners.begin(), listeners.end(), &l), 
			listeners.end());
	}

	void notify( IObject & object, const char *msg )
	{
		for( unsigned i = 0; i < listeners.size(); ++i )
			listeners[ i]->notify( Event( object, msg ));
	}
};


// If we don't do a cast here, then perhaps we can eliminate binding ?

struct IHelper
{
	virtual void operator () ( const Event & e) = 0; 
};

template< class C>
struct Helper : IHelper
{
	explicit Helper( C & c, void (C::*m)( const Event & e )) 
		: c( c),
		m( m)
	{ } 
	
	C & c; 
	void (C::*m)( const Event & e ); 

	void operator () ( const Event & e)
	{
		(c.*m)( e);
	}
};

// ok now. Do we want to put copy semantics

struct EventAdapter : INotify
{
	template< class C> 
	explicit EventAdapter( const char *predicate, C & c, void (C::*m)( const Event & e ) ) 
		: predicate( predicate),
		helper( new Helper< C>( c, m))
	{ }  

	~EventAdapter() 
	{ 
		delete helper;
	} 

	virtual void notify( const Event &e ) 
	{
		// there's no real efficiency advantage to handling
		// this here or in the dispatcher loop 
		if( !predicate 
			|| *predicate == 0 
			|| strcmp( predicate, e.msg) == 0) 
			helper->operator() ( e );
	}
private:
	IHelper * helper;
	const char *predicate;
};


//////////////////////////

/*

struct Object 
{
	Events events;

public:
	void notify( const char *msg)
	{
		events.notify( this, msg ); 
	}

	void register_( INotify & l) 
	{
		events.register_( l); 
	}
	void unregister( INotify & l) 
	{
		events.unregister( l);
	}
};

// ok, so this simplifies things a bit by moving the bind into the adaper, and getting rid
// of the placeholders ...

struct X 
{
	Object					& object;
	EventAdapter			x;


	X( Object & object)
		: object( object),
		x( "update", *this, & X::on_object_change)
	{ 
		object.register_( x );
	} 

	~X()
	{
		object.unregister( x );
	}
	
	void on_object_change( const Event & e ) 
	{
		std::cout << "object " << ((Object *)e.object) << " msg is " << e.msg << std::endl;
			// we can delegate manually
	}
};


int main()
{
	Object	object;
	X		x( object);

	object.notify( "update");
}

*/

