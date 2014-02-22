
#pragma once

#include <cassert>

// whether something gets wraped and composed - by a following system
// or else injected with references doesn't matter. they both create
// isolated composable systems with good event properties.

// we can do cancellable events as well.


struct INotify;

/*
	Different subsystems want events for different interfaces of the
	same object. The only way to do this is to downcast, which
	requires a base class.

*/

// change name notify() to emit() ?

struct IObject
{
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


struct Listeners
{
	// Change name to listeners

public:
	Listeners(); 
	~Listeners(); 
	void register_( INotify * l); 
	void unregister( INotify * l); 
	void notify( IObject & object, const char *msg ); 
private:
	struct Inner *d;
	Listeners( const Listeners & ); 
	Listeners & operator = ( const Listeners & ); 
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
		// there's a leak here, we really have to say that
		// we're using this object, but then it breaks const...	
		// ugghh...
//		l.add_ref();

		if( this == & l )
			return true;

		const NotifyAdapter< C> & h 
			= dynamic_cast< const NotifyAdapter< C> & > ( l);

		bool ret = &c == & h.c 
			&& m == h.m;

//		l.release();
		return ret;
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



