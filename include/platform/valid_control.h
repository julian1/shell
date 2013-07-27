/*
	The outmost layer of the onion is the platform specific gui layer. communication is probably going
	to need to be two-way.
*/
#pragma once

#include <common/ptr.h>

#include <service/valid_controller.h>

#include <gtkmm.h>



/*
	ok, the actual valid service is the thing that wants to be injected into everything ...


	http://www.pygtk.org/docs/pygtk/class-gtkadjustment.html

	Can set the adjustment in hours. and unit (step_increment )  in minutes ... 
	the increment is only shown when click on the little arrows (which are not showing).

*/

// we are going to have to put fucking interfaces over the place ...

struct GUIValidController  //: IValidController
{

	typedef GUIValidController this_type;

	GUIValidController( Gtk::Box & box , const ptr< IValidController> & valid_controller )
//		: count( 0 ),
		: box( box ),
		valid_controller( valid_controller ), 
		prev_button( "prev" ),
		next_button( "next" ),
//		label( "whoot" )
		adjustment( 0, 0, 1, 1), 
		scrollbar( adjustment )
	{
		std::cout << "$$$$ GuiValidController constructor" << std::endl;

		box.pack_end( next_button, Gtk::PACK_SHRINK  );
		box.pack_end( prev_button, Gtk::PACK_SHRINK  );

		box.pack_end( scrollbar, Gtk::PACK_SHRINK  );

		// 	the label should be removed from here. the param, model, runtime etc 
		// should all be able to be shown.  as a single thing. 
		//box.pack_end( label, Gtk::PACK_SHRINK  );

		// we need a button press callback ...

		//adjustment.signal_changed() .connect( sigc::mem_fun( *this, &this_type::on_my_value_changed) );
		adjustment.signal_value_changed() .connect( sigc::mem_fun( *this, &this_type::on_my_value_changed) );

		// pixels
		scrollbar.set_size_request( 100, -1 );

		prev_button.signal_clicked().connect( sigc::mem_fun( *this, &this_type::on_prev_button_clicked) );
		next_button.signal_clicked().connect( sigc::mem_fun( *this, &this_type::on_next_button_clicked) );

	}

	~GUIValidController()
	{ }

	void on_my_value_changed()
	{
		std::cout << "$$$$ whoot value changed" << std::endl;
		std::cout << adjustment.get_value () << std::endl;

		double uu = adjustment.get_value () ; 

		// OK this thing is going to need a lot more 

		ptr< Valid> start, finish;  
		valid_controller->get_min_max( start, finish ) ; 


		if( start->type == Desc::valid_simple )
		{
			//  using namespace boost::posix_time;
		    // using namespace boost::gregorian;

			assert( finish->type == Desc::valid_simple ); 
			boost::posix_time::ptime a = ((const ValidSimple &)*start ).valid ; 
			boost::posix_time::ptime b = ((const ValidSimple &)*finish).valid ; 

			std::cout << "start and finish " << a << " " << b << std::endl;

			// ok. So we want to convert to seconds etc. 

			boost::posix_time::time_duration diff = b - a;
/*
			std::cout << "duration " << diff  << std::endl;
			std::cout << "duration ms " << diff.total_milliseconds() << std::endl;
			std::cout << "duration s " << diff.total_seconds() << std::endl;
			std::cout << "duration min " << diff.total_seconds() / 60 << std::endl;
*/
			// use a 5 min increment ?

			int sec = (adjustment.get_value() / 1. ) * (diff.total_seconds() / 5 / 60 );  

			boost::posix_time::ptime new_valid = a + boost::posix_time::seconds( sec * 5 * 60 ); 

			

			std::cout << "new_valid " << new_valid << std::endl;

			valid_controller->set_valid( new ValidSimple( new_valid ) ) ; 
		} 
		else
			assert( 0 ) ; 

	}



	void on_prev_button_clicked()
	{
		std::cout << "prev button clicked" << std::endl;	
		valid_controller->change_valid( IValidController::prev );
	}
	void on_next_button_clicked()
	{
		valid_controller->change_valid( IValidController::next );
	}


//	void add_ref() { ++count; }
//	void release() { if( --count == 0 ) delete this; }

/*
	void add(  const ptr< IValidControllerJob> & job )
	{ 
		valid_controller->add( job );
	} 
	void remove(  const ptr< IValidControllerJob> & job )
	{ 
		valid_controller->remove( job );
	} 
*/

private:
//	unsigned count;
	Gtk::Box & box ; 
	ptr< IValidController> valid_controller; 
	Gtk::Button prev_button; 
	Gtk::Button next_button; 

	Gtk::Adjustment	adjustment;
	Gtk::HScrollbar scrollbar; 

//	Gtk::Label label ; 
};


