/*
	Modal control should selectively delegate events, if it preserves the concept of an active controller.
*/

/*
	This code is very weak. 
*/
/*
	Is there any point separating .cpp and .h here ?. It is only compiled once, and it is limited.  
	code is specific to platform anyway.

	OK. It is unclear, whether we may as well not bother to separate the instance from the interface. 
	since gui specific code is going to sit at the top of composition. and it's only going to be 
	instantiated once. 
*/
#pragma once

#include <common/ptr.h>

#include <service/position_editor.h>
#include <service/grid_editor.h>
#include <service/services.h>

#include <gtkmm.h>



struct ModalControl
{
	/*
		if we hold down ctrl, then could have multiple controllers active.
	*/
	/*
		Keeping the controller active/inactive is most flexible.
		We can keep multiple controller combinations to be active if we really want - 
			the point is to make it external to the controllers themselves. 

	*/

	// ok, this isn't great. but we can work with it. 

	// if we go into grid edit. then we potentially want to stop other items being moved. 
	// because it will interfere with the drag operation.

	// we could just pass a boost::function down.

	struct IBinder : RefCounted
	{
		virtual void set_active( bool value ) = 0; 
	};
#if 0
	template< typename T> struct Binder : IBinder
	{
		Binder( T & controller ) 
			: controller( controller)
		{ } 
		T & controller;
		void set_active( bool value )
		{
			controller.set_active( value ); 
		}
	}; 
#endif
	struct BinderGridEditor: IBinder
	{
		BinderGridEditor( IGridEditor & grid_editor ) 
			: grid_editor( grid_editor )
		{ } 
		IGridEditor & grid_editor; 
		void set_active( bool value )
		{
			grid_editor.set_active( value ); 
		}
	};

	struct BinderPositionEditor: IBinder
	{
		// not sure if this stuff should use ptr< IPositionEditor> ???

		BinderPositionEditor( IPositionEditor & position_editor ) 
			: position_editor( position_editor )
		{ } 
		IPositionEditor & position_editor; 
		void set_active( bool value )
		{
			position_editor.set_active( value ); 
		}
	};



	struct BinderNone : IBinder
	{
		BinderNone()  { } 
		void set_active( bool value ) { }
	}; 




	struct Mode : RefCounted
	{
		// if we made an interface, then we could inject labels, and other things without problem
		// actually it will need to be an interface if 

		typedef Mode this_type;

		Mode( ptr< Mode> & active_mode, const std::string &name, const ptr< IBinder> & binder )
			: name( name),
			binder( binder),
			active_mode( active_mode ),
			button(),
			active( false)
		{
			button.set_label( name  );
			//button.get_child().set_markup("<b>Submit</b>")
			//button.set_markup("<b>Submit</b>")
			button.set_relief( Gtk::RELIEF_NONE );
			//box.pack_start( button, Gtk::PACK_SHRINK  );

			button.signal_clicked().connect( sigc::mem_fun( *this, &this_type::on_button_clicked) );
		}

		std::string		name;
		ptr< IBinder>	binder;
		ptr< Mode>		& active_mode;	// we would have to make a reference
										// not convinced by any of this  - when a button is pressed, we should
										// get the name of the button	
		Gtk::Button		button;
		bool			active; 		

		Gtk::Widget &   widget()
		{
			return button; 
		}

		void on_button_clicked()
		{
			set_active( true);
		}

		void set_active( bool active_ )
		{
			if( active == active_ ) 
				return;

			active = active_;

			if( active)
			{
				std::cout << "set active" << std::endl;
				if( active_mode)
				{
					active_mode->set_active( false);
				}
				active_mode = this;

				button.set_relief( Gtk::RELIEF_NORMAL);
			}
			else
			{
				std::cout << "set inactive" << std::endl;
				button.set_relief( Gtk::RELIEF_NONE );
			}

			assert( binder);
			binder->set_active( active );
		}
	}; 


	ModalControl( Services & services, Gtk::HBox & box )
		: services( services),
		box( box )
	{
		//modes.push_back( new Mode( "grid edit", new Binder< GridEditor>( services.grid_editor ), & active_mode ) ); 
		modes.push_back( new Mode( active_mode, "grid edit", new BinderGridEditor( services.grid_editor ) ) ); 
		box.pack_start( modes.back()->widget(), Gtk::PACK_SHRINK  );

		modes.push_back( new Mode( active_mode, "position", new BinderPositionEditor( services.position_editor ) ) );	// possibly always on, unless (no several things want to turn off (grid edit, projection )
		box.pack_start( modes.back()->widget(), Gtk::PACK_SHRINK  );

		modes.push_back( new Mode( active_mode, "projection", new BinderNone ) ); 
		box.pack_start( modes.back()->widget(), Gtk::PACK_SHRINK  );

		// label.set_label( "label" );
		// box.pack_start( label, Gtk::PACK_SHRINK  );
		// box_Top.pack_end( box, Gtk::PACK_SHRINK);
	}

	// Ok, some services have a dispatch() and some have an update()
	// the dispatch ones are controllers. they should probably be factored.
	// and ones that just have update(), don't have to be in here.

	Services					& services;
	std::vector< ptr< Mode> >	modes;  
	ptr< Mode>					active_mode; 

	// Child widgets
	Gtk::HBox					& box;

/*
	Gtk::Widget &   widget()
	{
		return box; 
	}
*/
};





