
#pragma once

#include <gtkmm.h>

/*

	Example of control flow, using event model, bubbling up from Gtk,

	main	
		GridEditor
			ObjectWantingKeyboardEvents
				Keyboard
					Gtk::drawing_area
*/

/*
	Think it might be easier to let the subsystem hook events from a Keyboard
	object instead. Order of processing is important but can be determined by 
	instantiation order  

	To make the current approach work properly, we would need an abstract interface to 
	call on (that targets could implements), then we would have to  

	Mouse manager and Keyboard Manager both need to go in platform in any case.
	- The issue with events is that we only have an abstract untyped payload 
	- we could expose the key however

	int get_key(); 

	so we could query which would be sufficient...

	Putting this in Services could be useful, as it would allow any object to tap 
	keyboard events if it wanted.  Eg. an animated object.
*/

/*
struct IKeyboardTarget
{
	virtual void key_press( int ) = 0;  
	virtual void key_release( int ) = 0;  
};

	std::vector< IKeyboardTarget *>	listeners;
*/



struct Keyboard
{

	enum { shift_key = 65505, ctrl_key = 65507 } ;  


	typedef Keyboard this_type; 

	Keyboard( Gtk::Window & window ,
		GridEditor		& grid_editor,
		PositionEditor	& position_editor
	) : window( window ),
		grid_editor( grid_editor),
		position_editor( position_editor)
	{

		window.signal_key_press_event() .connect( sigc::mem_fun( *this, &this_type::on_key_press_event));
		window.signal_key_release_event() .connect( sigc::mem_fun( *this, &this_type::on_key_release_event));
	}

	Gtk::Window		& window ; 
	GridEditor		& grid_editor;
	PositionEditor	& position_editor;


	static int translate_code( unsigned code )
	{
		int ret = 0; 
		switch( code)
		{
			// case GDK_Return: case GDK_Escape: case GDK_F1: etc
			case GDK_KEY_Shift_L  :  ret = this_type::shift_key; break; 
			case GDK_KEY_Control_L: ret = this_type::ctrl_key; break;
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
		return false;
	}


	bool on_key_release_event(GdkEventKey* event )
	{
		grid_editor.key_release( translate_code( event->keyval ) ); 
		position_editor.key_release( translate_code( event->keyval ) ); 
		return false;
	}
};


