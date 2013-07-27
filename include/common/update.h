
#ifndef MY_UPDATE_H
#define MY_UPDATE_H

struct UpdateParms
{
	unsigned dt;	// in milliseconds


};


#if 0

#include <common/intrusive_ptr.h> 
#include <common/event_store.h> 

/*
	the only alternative to computing the deps, is to have every dep pushed into a subsys, 
	and then sequence the subsys order.

	Having sequenced update(), has the extremely nice property that we don't have to 
	sit there listening for events, but can just query data, when we require it during update().

	It's tell don't ask.

	Querying a child for data using an add()/remove() interface allows individual 
*/


#include <set>
#include <map>
#include <vector>

/*
	I think we can actually manage the recursion ... without first calculating all the deps
*/

struct IUpdate; 

struct UpdateContext
{
	std::set< IUpdate *>	already_seen; 

	bool fast; 
	// bool clear		

	// if we are calculating...
	std::map< IUpdate *, std::vector< IUpdate *> > deps; 
};



// because the recursion is a bit complicated ... - it might make sense to use an enum type context, and then switch 
// on the operation. 

struct IUpdate
{
	virtual void calc_deps( std::map< IUpdate *, std::vector< IUpdate *> > & deps ) = 0;	// internals of algorithm should be hidden 

	virtual void update() = 0; 

	// virtual void set_dirty( bool )
	// virtual void clear_dirty() 	
	// OK, should this be the point where ...

	// we put these on here rather than a seperate interface, so that a source only has 
	// to maintain a reference to IUpdate
	// actually think we need a different interface.
	virtual void subscribe( IEventStore & es ) = 0;
	virtual void unsubscribe( IEventStore & es ) = 0;
};

// Lets try and

#endif



//	virtual void add_ref() = 0;
//	virtual void release() = 0;

//	virtual void visit( IUpdateor * src ) = 0;	// internals of algorithm should be hidden 
//	std::map< IUpdate *, std::vector< IUpdate *> > & deps;



// no, the potential reason to use a visitor for this, is to move logic outside
// the classes ... 
/*
	- we have to gather the dependencies ..., which is an algorithm that applies across 
	all the objects

	- if we have getters and setters for deps then we can correctly trace 
*/

#if 0
struct IUpdateor
{
	virtual void operator () ( struct Renderer * ) = 0 ; 
	virtual void operator () ( struct SimpleContourRenderer* ) = 0 ; 
	virtual void operator () ( struct ContourSource * ) = 0 ; 

	// we are visiting the internals that are hidden away in the fucking cpp file...
	// this isn't going to work. 

/*
	virtual void operator () ( struct IContourer * ) = 0 ; 
	virtual void operator () ( struct IGridSource * ) = 0 ; 
	//virtual void operator () ( struct IRenderer * ) = 0 ; 
	virtual void operator () ( struct IContour * ) = 0 ; 
*/
};
#endif
#endif
