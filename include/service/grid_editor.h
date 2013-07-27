
#pragma once

#include <common/grid.h>
#include <common/ui_events.h>	



struct IGridEditorJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	// it's ok not to have the virtual destructor, if we are never going to call the destructor from here. 
	// the destructor will normally get called from release(); 
	// virtual ~IGridEditorJob() = 0;  

	virtual ptr< Grid> get_grid() = 0;
	virtual void set_grid( const ptr< Grid> & grid )  = 0;

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




struct GridEditor : IGridEditor, IMyEvents
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


