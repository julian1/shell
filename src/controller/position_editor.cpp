
#include <controller/position_editor.h>
#include <common/ui_events.h>

#include <iostream>
#include <cassert>
#include <set>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

/*
	- Really don't see that we want a modal position editor.
	- if we don't want the grid or projection area to be moved, then don't respond to 
	the hit-test! 

*/

namespace {

	typedef std::set<  IPositionEditorJob * >	objects_t;

};


struct PositionEditorImpl 
{
	PositionEditorImpl()
//		: active( false),
		: active_object( NULL ) 
	{ } 


	// unsigned count;

//	bool	active;					// is the controller active

	objects_t	objects;			// change name to job ? 
									// it's really not an object, it's an abstraction job

	// the problem with weak references is that it's not possible to test them.
	IPositionEditorJob  * active_object; 

	int mouse_down_x; 
	int mouse_down_y; 
};


PositionEditor:: PositionEditor()
	: d( new PositionEditorImpl ) 
{ } 

PositionEditor::~PositionEditor()
{ 
	delete d;
	d = NULL;
}  





void PositionEditor::add(  IPositionEditorJob  & job )
{ 
	d->objects.insert( & job );
} 

void PositionEditor::remove( IPositionEditorJob  & job )
{ 
	d->objects.erase( & job );
} 


// IMyEvents
void PositionEditor::mouse_move( unsigned x, unsigned y ) 
{ 
//	if( !d->active)
//		return;

	if( ! d->active_object )
		return;

	// int dx = event.x - mouse_down_x ; 
	// int dy = event.y - mouse_down_y ; 

	// this isn't going to work . 
	// actually we can update from the last position

	d->active_object->move( d->mouse_down_x, d->mouse_down_y, x, y ); 

	d->mouse_down_x = x;		
	d->mouse_down_y = y;		

} 
void PositionEditor::button_press( unsigned x, unsigned y )  
{ 
//	if( !d->active)
//		return;

	// find the closest object
	// loop and his hittest and find best object / job
	IPositionEditorJob * closest_object = NULL; 
	double closest_dist = 1234; 
//	foreach( const objects_t::value_type & object, d->objects )
	foreach( IPositionEditorJob * job, d->objects )
	{
		//IPositionEditorJob & job = * object.second ;
		double dist = job->hit_test( x, y ); 
		if( dist < closest_dist )  
		{
			closest_object =  job;
			closest_dist = dist; 
		}
	}

	if( closest_dist < 5 && closest_object )
	{
		std::cout << "found an item " << closest_dist << std::endl;
		d->active_object = closest_object;
		d->active_object->set_active( true );	
	}
	else
	{
		std::cout << "nothing close" << std::endl;
		d->active_object = NULL; 
	}

	d->mouse_down_x = x;		
	d->mouse_down_y = y;		
} 


void PositionEditor::button_release( unsigned x, unsigned y )  
{ 
//	if( !d->active)
//		return;

	// end the transaction 
	if( d->active_object )
	{
		d->active_object->finish_edit();	// transaction boundary
		d->active_object->set_active( false );	
		// clear
		d->active_object = NULL;
	}
}

void PositionEditor::key_press( int )
{ } 
void PositionEditor::key_release( int )  
{ } 



void PositionEditor::set_active( bool active_ )
{ 
#if 0
	d->active = active_;

	int sz = d->objects.size(); 

	// alert all components	
	foreach( IPositionEditorJob  * job , d->objects )
	{
		assert( sz == d->objects.size() ); 
//		const ptr< IPositionEditorJob>   & job = object.second ;

		//objects.find( job); 
		//assert( objects.find( object.first ) != objects.end() ) ;

		// we can't call on the object because 
		job->set_position_active( d->active );
	}	

#endif
} 


