
#ifndef MY_EVENT_SOURCE_H 
#define MY_EVENT_SOURCE_H 

#include <boost/intrusive_ptr.h>

// basically just like

// to replay events we have to have a common target

/*
	- events should be nice simple state, with no logic, such that they can be easily serialized.
	they operate on entities.
	- they avoid logical compression, and represent the complete set of state changes over time

	- we may be able to delegate, just using uid, rather than make the root handle everything. 
		- actually it is probably better to direct explicitly. 
*/

/*
	Note that the checkpoint is actually going to be the aggregate root itself.
*/

struct ESEvent
{
	enum type_t { 
		none, 
		grid_edit_source_create,  
		grid_edit_source_destroy,  
		grid_edit_source_finished_edit,  


		set_active,		// active.h




		////// NON STORABLE EVENTS
		// not sure if we are using these
		contour_add, 
		contour_remove,
		projected_contour_add, 
		projected_contour_remove,
		styled_contour_add, 
		styled_contour_remove,




	}; 

	ESEvent( type_t type ) 
		: count( 0),
		type( type),
		uid( 0) 
	{ } 

	virtual ~ESEvent( )  
	{ } ;

	void add_ref()  { ++count; } 
	void release()  { if( --count == 0) delete this; } 
private:

	unsigned count; 
public:
	unsigned	uid;	// change name entity_uid
	type_t		type;
private:
	ESEvent( const ESEvent & );  
	ESEvent& operator = ( const ESEvent & );  
};


// ok, so if we are registering this on a target ...
// it's both a source and a target ? depending on which side
struct IEventStore		// should be called a store
{
	//virtual void dispatch( const ptr< ESEvent>  &  ) = 0; 

	// SHOULD BE CALLED STORE, not apply 

	virtual void store( const ptr< ESEvent> & ) = 0; 
};












#if 0
struct IEventRegister : IUpdate
{
	// this is a IConnectionPoint ...

	// OK - This can now be the source of any events  grids, isolines, styled isolines.
	// probably needs add_ref and release

	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void subscribe( IEventStore & ) = 0;  
	virtual void unsubscribe( IEventStore & ) = 0;  
};
#endif


// we would filter some events, at point they are generated   ...


// the aggregate root would implement this ...
// and then insert into all subordinate objects


#endif

