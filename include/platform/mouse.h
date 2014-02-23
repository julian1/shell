
#pragma once

#include <gtkmm.h>

/*	void mouse_move( unsigned x, unsigned y ) ;
	void button_press( unsigned x, unsigned y );  
	void button_release( unsigned x, unsigned y );  
*/


struct MouseManager
{
	MouseManager( Gtk::DrawingArea	& drawing_area,
		GridEditor		& grid_editor,
		PositionEditor	& position_editor
	) 	
		: drawing_area( drawing_area ),
		grid_editor( grid_editor),
		position_editor( position_editor)
	{
		typedef MouseManager this_type; 

		drawing_area.signal_motion_notify_event() .connect( sigc::mem_fun( *this, &this_type::on_motion_notify_event));
		drawing_area.signal_button_press_event() .connect( sigc::mem_fun( *this, &this_type::on_button_press_event));
		drawing_area.signal_button_release_event() .connect( sigc::mem_fun( *this, &this_type::on_button_release_event));
	}

	Gtk::DrawingArea	& drawing_area;
	GridEditor		& grid_editor;
	PositionEditor	& position_editor;

	bool on_motion_notify_event( GdkEventMotion *event )
    {
//		std::cout << "mouse move event" << std::endl;
		grid_editor.mouse_move( event->x, event->y ); 
		position_editor.mouse_move( event->x, event->y ); 
		return false;
	}

	bool on_button_press_event( GdkEventButton* event)
	{
		grid_editor.button_press( event->x, event->y ); 
		position_editor.button_press( event->x, event->y ); 
		return false;   
	}
	bool on_button_release_event( GdkEventButton* event)
	{
		grid_editor.button_release( event->x, event->y ); 
		position_editor.button_release( event->x, event->y ); 
		return false; 
	}
};


