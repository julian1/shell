
#pragma once

#include <common/grid.h>



struct IGridEditorJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	// it's ok not to have the virtual destructor, if we are never going to call the destructor from here. 
	// the destructor will normally get called from release(); 
	// virtual ~IGridEditorJob() = 0;  

	virtual ptr< Grid> get_grid() = 0;
	virtual void set_grid( const ptr< Grid> & grid )  = 0;

/*
	Can't we do the transform outside here ?

	Probably not because this is going to have 

	mouse/keyboard -> position editor (set active) -> grid editor ->  object

	Actually I think the two editors need to operate completely independently -
		both responding to mouse/keyboard events. 

		otherwise the functionality will need to be placed on the object.

	- Remember that it was modal before, which is what we are trying to get away from.
	but we probably need to have a way of cancelling events.

	mouse/keyboard - edit multiplex -> position editor
									-> grid editor 

		(in this situation, we have to inform the multiplexer which editor is active
		to avoid the propagation of events )
	--

	We could have a single hittest controller - 
		if the object is active it raises an event to inform the controller it's active.  
	
		then the mouse/keyboard event dispatcher could route events only to that 
		active controller.


		Having the object inform the controller that it's active is nice. It means
		we could use tab to switch between objects etc.

	Call it the object_selector or selector_controller 

	- The other controllers (position/grib) could still directly work with mouse/keyboard
	but would only do so if they were active.

	- alternatively why not have the controllers working together simultaneously. doing
	hit testing, and operations. 

	- we have to prevent them conflicting - cancelling an event would actually be simplest. 
		First that wants to use the event, cancels it.

	- Rather than switching, or boolean testing.

	- Cancelling won't work beucase second controller cannot cancel first controller's events.

	- are we sure we shouldn't route events straight to an active object - and let
	it be responsible for doing stuff?.
		( rather than indirectly to a controller )

		- to a grid
		- or a projection
		- or a control point 

	One Controller that is lightweight and has a concept of the active object to
	which it routes stuff.  
*/

	// project, affine and georeference, for the grid with reference to mouse coordinates
	// eg. GridEditor should not know either
	// will have to invert
	virtual void transform( double *x, double *y ) = 0 ; 

	// This is extremely good method to indicate to the aggregate root to simplify the drawing for speed
	// to draw or not draw etc.
	virtual void set_active( bool ) = 0;

	// set_grid_dirty (which can just be a bool on the aggregate root), it doesn't have to be on the Grid	
	// set_active( bool )
};

// ok, the grid editor needs to be able to see if the grid has changed ...
// But the only way a grid can broadcast events ... is if it has it's peer ...


struct IGridEditor
{
	virtual void add(  const ptr< IGridEditorJob> & job ) = 0; 
	virtual void remove(  const ptr< IGridEditorJob> & job ) = 0; 

	/* IMPORTANT
	this should be removed from here. Only Modal control requires it.
	Additionally. Modal control can preserve the active/ not active status
	and directly route/delegate the events.
	and probably this as well ...
	*/
	virtual void set_active( bool ) = 0; 
};




struct GridEditor : IGridEditor//, IMyEvents
{
	// specific impl, that also supports the ui_events inteface 
	// the IMyEvents is only relevant to gui, and so is not exposed everywhere. 

	// lifetime
	GridEditor(); 
	~GridEditor(); 
	
	// IGridEditor
	void add(  const ptr< IGridEditorJob> & job ); 
	void remove(  const ptr< IGridEditorJob> & job ); 
	//void dispatch( const UIEvent & e); 
	void set_active( bool ); 

	// IMyEvents
	void mouse_move( unsigned x, unsigned y ) ;
	void button_press( unsigned x, unsigned y );  
	void button_release( unsigned x, unsigned y );  
	void key_press( int );  
	void key_release( int );  

private:
	struct GridEditorInner *d; 
	GridEditor( const GridEditor & );  
	GridEditor& operator = ( const GridEditor & );  
};


