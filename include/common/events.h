
#pragma once

#include <cassert>

// whether something gets wraped and composed - by a following system
// or else injected with references doesn't matter. they both create
// isolated composable systems with good event properties.

// we can do cancellable events as well.

// this is a version without bind or function
// it relies on the event handler to cast to the
// correct type

//#include <boost/bind.hpp>
//#include <boost/function.hpp>

struct INotify;

/*
	Different subsystems want events for different interfaces of the
	same object. The only way to do this is to downcast, which
	requires a base class.

*/

// change name notify() to emit()

struct IObject
{
	// events
	virtual void register_( INotify * ) = 0;
	virtual void unregister( INotify * ) = 0;

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
	virtual void add_ref() = 0;
	virtual void release() = 0;
	virtual void notify( const Event &e ) = 0;
	virtual bool operator == ( const INotify & ) const = 0;
};


struct Events
{
	// Change name to listeners

public:
	Events(); 
	~Events(); 
	void register_( INotify * l); 
	void unregister( INotify * l); 
	void notify( IObject & object, const char *msg ); 
private:
	struct Inner *d;
	Events( const Events & ); 
	Events & operator = ( const Events & ); 
};



template< class C>
struct NotifyAdapter : INotify
{
private:
	int count;
	C & c;
	void (C::*m)( const Event & e );

	NotifyAdapter( const NotifyAdapter & );
	NotifyAdapter & operator = ( const NotifyAdapter & );

public:
	explicit NotifyAdapter( C & c, void (C::*m)( const Event & e ))
		: count( 0),
		c( c),
		m( m)
	{ }

	void add_ref() {
		assert( count >= 0);
		++count;
	}

	void release() {
		assert( count > 0);
		if( --count == 0 )
			delete this;
	}

	void notify( const Event &e )
	{
		(c.*m)( e);
	}

	bool operator == ( const INotify & l ) const
	{
		if( this == & l )
			return true;

		const NotifyAdapter< C> & h 
			= dynamic_cast< const NotifyAdapter< C> & > ( l);

		return &c == & h.c 
			&& m == h.m;
	}
};


template< class C>
NotifyAdapter< C> * make_adapter( C & c, void (C::*m)( const Event & e ))
{
	return new NotifyAdapter< C>( c, m );
}

template< class C>
NotifyAdapter< C> * make_adapter( C * c, void (C::*m)( const Event & e ))
{
	return new NotifyAdapter< C>( *c, m );
}



/*
// If we don't do a cast here, then perhaps we can eliminate binding ?

struct IHelper
{
	virtual void operator () ( const Event & e) = 0;
//	virtual bool operator == ( const IHelper & x) = 0;
};
*/


/*
// ok now. Do we want to put copy semantics

struct NotifyAdapter : INotify
{
	template< class C>
	explicit NotifyAdapter( const char *predicate, C & c, void (C::*m)( const Event & e ) )
		: predicate( predicate),
		helper( new Helper< C>( c, m))
	{ }

	explicit NotifyAdapter( )
		: predicate( 0 ),
		helper( 0)
	{ }

	explicit NotifyAdapter( const NotifyAdapter & adapter )
	{
		// Copy and take ownership of memory
		predicate = adapter.predicate;
		helper = adapter.helper;
		adapter.predicate = 0;
		adapter.helper = 0;
	}

	void operator = ( const NotifyAdapter & adapter )
	{
//		std::cout << "here" << std::endl;
//		exit( 0);

		// Copy and take ownership if memory
		predicate = adapter.predicate;
		helper = adapter.helper;
		adapter.predicate = 0;
		adapter.helper = 0;
	}



	bool operator == ( const NotifyAdapter & adapter )
	{
		assert( 0 );
		// we don't know what to cast too here ...
		//return *helper == *adapter.helper;
	}

	~NotifyAdapter()
	{
		if( helper)
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
	mutable IHelper * helper;
	mutable const char *predicate;
};

*/


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
	NotifyAdapter			x;


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

