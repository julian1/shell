
#pragma once

#include <gtkmm.h>


/*
	Think it might be easier to let the subsystem hook the Keyboard
	events. Order of processing is important but can be determined by 
	instantiation order  

	To make these work properly, we would need an abstract interface to 
	call on, then we would have to  

	Both of these need to be p


	Mouse manager and Keyboard Manager both need to go in platform in any case.
*/

struct KeyboardManager
{
	KeyboardManager( Gtk::Window & window ,
		GridEditor		& grid_editor,
		PositionEditor	& position_editor //,
//		IRenderSequencer & render_control
	) : window( window ),
		grid_editor( grid_editor),
		position_editor( position_editor)//,
//		render_control( render_control )
	{
		typedef KeyboardManager this_type; 

		window.signal_key_press_event() .connect( sigc::mem_fun( *this, &this_type::on_key_press_event));
		window.signal_key_release_event() .connect( sigc::mem_fun( *this, &this_type::on_key_release_event));
	}

	Gtk::Window		& window ; 
	GridEditor		& grid_editor;
	PositionEditor	& position_editor;
//	IRenderSequencer & render_control; 


	static int translate_code( unsigned code )
	{
		int ret = 0; 
		switch( code)
		{
			// case GDK_Return: case GDK_Escape: case GDK_F1: etc
			case GDK_KEY_Shift_L  :  ret = IMyEvents ::shift_key; break; 
			case GDK_KEY_Control_L: ret = IMyEvents ::ctrl_key; break;
			default: ret = code; break;
		};
		return ret; 
	}
	// now we want keyboard events. to enable pan
	bool on_key_press_event( GdkEventKey* event )
    {
		/*
		std::cout << "keypress event " << event->hardware_keycode << " " << char( event->hardware_keycode) << std::endl;
		std::cout << "keypress event " << event->keyval << " " << char( event->keyval ) << std::endl;
		*/

		grid_editor.key_press( translate_code( event->keyval ) ); 
		position_editor.key_press( translate_code( event->keyval ) ); 

//		render_control.signal_immediate_update(  ); 
		return false;
	}
	bool on_key_release_event(GdkEventKey* event )
	{
		grid_editor.key_release( translate_code( event->keyval ) ); 
		position_editor.key_release( translate_code( event->keyval ) ); 

//		render_control.signal_immediate_update(  ); 
		return false;
	}
};


