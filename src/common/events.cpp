
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
	Inner()
	{ }


	std::vector< INotify *> listeners;
};





Events::Events()
	: d( new Inner)
{ } 

Events::~Events()
{
	delete d;
	d = NULL;
}



void Events::register_( INotify * l)
{
	l->add_ref();
	d->listeners.push_back( l);
}

void Events::unregister( INotify * l)
{
	// we will have to change teh comparison operation here,
	// if we do equality stuff...

	// this is a pointer comparison, but we need a predicate comparison...

	d->listeners.erase(
		std::remove_if( d->listeners.begin(), d->listeners.end(),  X( l) ),
		d->listeners.end()
	);

	l->release();
}

void Events::notify( IObject & object, const char *msg )
{
	for( unsigned i = 0; i < d->listeners.size(); ++i ) {
		d->listeners[ i]->notify( Event( object, msg ));
	}
}


