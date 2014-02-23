
#pragma once 

/*
	4 moveable objects at the moment

	anim object
	grid
	proj
	contour
*/




struct IPositionEditorJob
{
	// - return distance so that position editor can choose when several options 
	// - this can handle its own projection needs, and a projection wrapper can 
	// be added. also georef etc. eg. it can delegate
	virtual double hit_test( unsigned x, unsigned y ) = 0; 
	
	virtual void set_active( bool ) = 0; 	

	// eg. we have move, and resize operations, with different control points, 
	// so it could be useful to distinguish this.
	// draw control points etc.
	virtual void set_position_active( bool ) = 0; 	

	//virtual void move( int dx, int dy ) = 0 ; 
	virtual void move( int x1, int y1, int x2, int y2 ) = 0 ; 

	// it may be useful to have a begin_edit. in order to change geom or color 
	// no. capture it in set_active(0

	virtual void finish_edit() = 0;				// to push the model event 
};



struct IPositionEditor
{
	virtual void add(  IPositionEditorJob & job ) = 0; 
	virtual void remove( IPositionEditorJob  & job ) = 0; 

	// REMOVE Me !!! or place out
	// should not be here. put in modal contrl.
	virtual void set_active( bool ) = 0; 
};



struct PositionEditor : IPositionEditor//, IMyEvents
{
	// lifetime
	PositionEditor();
	~PositionEditor();

	// IPositionEditor
	// change names to just add_job() and remove_job()
	void add( IPositionEditorJob  & job ) ; 
	void remove( IPositionEditorJob  & job ) ; 


	// REMOVE Me !!! or place out
	void set_active( bool );	

	// IMyEvents
	void mouse_move( unsigned x, unsigned y ) ;
	void button_press( unsigned x, unsigned y );  
	void button_release( unsigned x, unsigned y );  

	void key_press( int );  
	void key_release( int );  

private:
	struct PositionEditorImpl *d;
	PositionEditor( const PositionEditor & );
	PositionEditor & operator = ( const PositionEditor & );
};



