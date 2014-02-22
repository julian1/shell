
#pragma once

#include <common/ptr.h>

#include <controller/level_controller.h>

#include <gtkmm.h>



/*
	ok, the actual level service is the thing that wants to be injected into everything ...
*/

// we are going to have to put fucking interfaces over the place ...

struct GUILevelController  
{

	typedef GUILevelController this_type;

	GUILevelController( Gtk::Box & box , const ptr< ILevelController> & level_controller )
//		: count( 0 ),
		: box( box ),
		level_controller( level_controller ), 
		up_button( "up" ),
		down_button( "down" ),
		label( "whoot" )
	{
		std::cout << "$$$$ GuiILevelController constructor" << std::endl;

		box.pack_end( up_button, Gtk::PACK_SHRINK  );
		box.pack_end( down_button, Gtk::PACK_SHRINK  );
		

		// 	the label should be removed from here. the param, model, runtime etc 
		// should all be able to be shown.  as a single thing. 
		box.pack_end( label, Gtk::PACK_SHRINK  );

		// we need a button press callback ...

		up_button.signal_clicked().connect( sigc::mem_fun( *this, &this_type::on_up_button_clicked) );
		down_button.signal_clicked().connect( sigc::mem_fun( *this, &this_type::on_down_button_clicked) );
	}

	~GUILevelController()
	{
		std::cout << "$$$$ GuiILevelController destructor" << std::endl;
	}


//	void add_ref() { ++count; }
//	void release() { if( --count == 0 ) delete this; }

	void on_up_button_clicked()
	{
		std::cout << "up button clicked" << std::endl;	
		level_controller->change_level( ILevelController:: up );
	}
	void on_down_button_clicked()
	{
		level_controller->change_level( ILevelController:: down );
	}

	void add(  const ptr< ILevelControllerJob> & job )
	{ 
		level_controller->add( job );
	} 
	
	void remove(  const ptr< ILevelControllerJob> & job )
	{ 
		level_controller->remove( job );
	} 


private:
//	unsigned count;
	Gtk::Box & box ; 
	ptr< ILevelController> level_controller; 
	Gtk::Button up_button; 
	Gtk::Button down_button; 
	Gtk::Label label ; 
};


