
#include <common/events.h>

#include <vector>
#include <algorithm>
#include <iostream>
//#include <cstring>

namespace {

	struct X
	{
		// compare objects, make generic and a template

		X( const INotify *a )
			: a( a)
		{ } 

		const INotify *a;

		bool operator () ( const INotify *b) const
		{
			return *a == *b;
		}
	};

};


struct Inner
{
	typedef std::vector< INotify *> listeners_type;

	listeners_type  listeners;
};


Listeners::Listeners()
	: d( new Inner)
{ } 

Listeners::~Listeners()
{
	delete d;
	d = NULL;
}

void Listeners::register_( INotify * l)
{
	l->add_ref();
	d->listeners.push_back( l);
}

void Listeners::unregister( INotify * l)
{
	d->listeners.erase(
		std::remove_if( d->listeners.begin(), d->listeners.end(),  X( l) ),
		d->listeners.end()
	);

	l->release();
}

void Listeners::notify( IObject & object, const char *msg )
{
	typedef Inner::listeners_type listeners_type;

	for( listeners_type::iterator i = d->listeners.begin(); 
		i != d->listeners.end(); ++i ) {

		(*i)->notify( Event( object, msg ));
	}
}


