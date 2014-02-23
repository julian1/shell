
#include <controller/grid_editor.h>	// Grid

#include <iostream>
#include <cmath>	// pow, sqrt	
#include <cstdlib>	// NULL
#include <cassert>
#include <vector>
#include <set>


//#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH


namespace {

static double pow2(double x)
{
	return x * x; 
}

/*
	we have a double sqrt and pow and division - for every pix 
	300x300 = 90000 - it ought to be ok ? 
*/

struct gaussf
{

	//static double gaussf( double x, double c)
	double operator () ( double x, double c)
	{
		// seems to flat but peak should == 1
		//f(x) = a e  -(x - b) 2 /c
		const double a = 1;			// height
		const double b = 0;			// centre 
		// const double c = 1500;	// spread 
		//	const double c = 100;		// spread 
		const double e = 2.718281828;

		return a * ( pow( e, - pow2( x - b) / c));  
	}
};


// change name to gaussf_cached
struct gaussf2
{

	std::vector< double>	values;

	gaussf			f; 

	unsigned		wn;

	gaussf2()
		: wn ( 500)
	{
		for( unsigned i = 0; i <= wn * 3; ++i)
		{

			double weight = sqrt( f( double( i) / wn, 1));
			values.push_back( weight );  
		}	 

//		std::cout << "computed values" << std::endl;
	}

	// there is something funheight() with the spread in the original func

	//static double gaussf( double x, double c)
	double operator () ( double x, double c)
	{
//		assert( x >= 0);

		double limit = x * wn * .001; 

		if( limit >=  values.size() ) 
			return 0;

//		return f( x, c);
//		std::cout << "retu" << std::endl;	
		return values[ limit];
	}
};



class filter_bicubic
{   
	static double pow3(double x)
	{   
		return (x <= 0.0) ? 0.0 : x * x * x;
	}   

public:
	static double radius() { return 2.0; }
	static double calc_weight(double x)
	{   
		return
			(1.0/6.0) * 
			(pow3(x + 2) - 4 * pow3(x + 1) + 6 * pow3(x) - 4 * pow3(x - 1));
	}   
};  


template< class M, class F > 
struct ConvolutionAdaptor
{
	// matrix data source
	const M					&m;			
	
	// our filter function 
	const F					filter;

	// mapping of weights using squared distances
	// to avoid sqrt 
	std::vector< double>	weights;
	
	// resolution of weight loopup table 
	signed					wn; 

	ConvolutionAdaptor( const M &m)
		: m( m)
		, filter()
		, wn( 500)
	{ 
		assert( filter.radius() == 2);

		// build lookup table using the filter - weight for (distance ^ 2)
		for( unsigned i = 0; i <= 4 * wn; ++i)
		{
			double dist = sqrt( double( i) / wn);
			weights.push_back( filter.calc_weight( dist) );
		}
	} 

	static double pow2(double x)
	{
		return x * x; 
	}

	double operator () ( double x1, double y1) const
	{
		// compute 3x3 matrix bounds
		signed start_x = floor( x1) - 1;
		signed start_y = floor( y1) - 1;
		signed end_x = start_x + 4;			// one past end
		signed end_y = start_y + 4; 

		// clamp 3x3 bounds to ensure within matrix bounds
		if( start_x < 0)  start_x = 0;
		if( start_y < 0)  start_y = 0;
		if( end_x > m.width()) end_x = m.width(); 
		if( end_y > m.height()) end_y = m.width(); 

		// sum of weights
		double sum_w = 0;
		// sum of product of weight and value
		double sum_wv = 0;

		// hmmm 1000x1000x12 - is only 12 million - it should be quicker ? 
		// and we are actually only doing a total subset of this eg 150x150 ?? or 300x300

		// very tight inner loop - normally iterating 3x3 
		for( unsigned y = start_y; y < end_y; ++y)
		for( unsigned x = start_x; x < end_x; ++x)
		{
			// compute the distance - leave as square avoiding expensive sqrt
			double dist2 = pow2( y1 - y) + pow2( x1 - x); 
		
			// eg (dist2 = dist * dist)		
			if( dist2 <= 4) 
			{ 
				// double w = weights.at( dist2 * wn);
				double w = weights[ dist2 * wn];
				// double w =  filter.calc_weight( sqrt( pow2( y1 - y) + pow2( x1 - x)));
				// log << "w " << w << " w2 " << w2 << std::endl;
				double v = m( x, y); 
				sum_wv += v * w;
				sum_w += w; 
			}
		}
		
		return sum_wv / sum_w; 
	}
};





static Grid * do_stuff( const Grid * mouse_down_model, double x1, double y1, double x2, double y2)
{
/*
	at 300x300 it is quite good resolution
	
	- sometimes it calculates (10ms) and draws (10ms) really quickly but then sometimes it sticks

	- HANG ON WE SHOULDNT BE DOING COMPUTATIONALLY INTENSIVE STUFF EXCEPT
	in the DRAW OTHERWISE IT QUEUES MESSAGES ...

	- perhaps we have to store the entire calculation in the command and run it ? 
	- BUT THAT WILL FUCK UP THE TREE TRAVERSAL FOR WHAT HAS CHANGED ...
	UGGHH

	- there is still the stickiness 30/40ms sometimes 

	-- BUT we can draw at high and low resolutions for mouse down and up

*/
//	start_timer();

	// ok so we want to generate a matrix that is the same size
	// as mouse_down_model with lookbacks computed with a gausian value.

	// we create a copy of the WE SHOULD COPY HERE ... so that original is untouched ...	

	//assert( in_drag);

//	Matrix						new_model;

//	Matrix		new_model( mouse_down_model.width(), mouse_down_model.height());
	Grid * new_model = new Grid( mouse_down_model->width(), mouse_down_model->height() ); 

	//agg::trans_affine	t;

	ConvolutionAdaptor< Grid, filter_bicubic>		a( *mouse_down_model);
/*
	we can thread this using interlacing 

	perhaps we should also apply the same trick with the gaussf function
	of caching the lookup and avoiding the sqrt ? 
	- it is a bit trickier because of the spread argument
*/

//	gaussf	gauss;
	gaussf2	gauss2;

	for( signed y = 0; y < mouse_down_model->height(); ++y) 
	for( signed x = 0; x < mouse_down_model->width(); ++x) { 

		// use squared lookup for this ?
		// gaussf should be moved into a function object aheight()way
//		double dist = sqrt( pow2( x2 - x) + pow2( y2 - y)); 
		double dist2 =  pow2( x2 - x) + pow2( y2 - y); 
		double weight = gauss2( dist2, 100);


		// this is the position for the value we want
		double gx = x - (x2 - x1) * weight;
		double gy = y - (y2 - y1) * weight;
		
		/*
		our adaptor can actually interpolate (extrapolate) outside the 
		data bounds - so i think we should probably ask it to
		(note that adaptor can do the fast lookup if <4, otherwise
		reverting to explicit calc ?? - not sure ) ?  
		- not sure it creates a 3x3 table for the interpolator ...
		then being outside the data will fail. hmm
		 we might need something
		*/
		//if( gx1 >= 0 && gx1 < m.width() && gy1 >= 0 && gy1 < m.height())
		if( weight > 0.005 
			&& gx >= 0 && gx < mouse_down_model->width() 
			&& gy >= 0 && gy < mouse_down_model->height())
		{
			//new_model.set( x, y) = a( gx, gy);
			(*new_model)( x, y) = a( gx, gy);
		} 


		else 
		{ 
			(*new_model)( x, y) = (*mouse_down_model)( x, y);
			
			// reaching outside ? - we have to do somethign
			// cannot do the reach - and there is not a smooth transition from edge
			// analyze x,y and take the border case ?
			// 1) limit ability of user to edit if near edge
			// 2) run extrapolation adaptor ? 
			// 3) use mouse down data ? 
			// 4) adjust the gaussian model so that it more rapidly retreats toward edges ? 
			// 5) oversize the actual editable data - and compute an extrapolation for the boundary ??
			// 6) adjust weighting system  so decrease towards edge ? 
			//
			// 7) use a SLOPE along the positioning vector
			// actually we have a vector - so we could take two nearby values get the slope and project them
			// (x1,y1) and (x2,y2) -> 
			// remember we just want a value ? 
			// we know we have x,y as a valid value ...
			//
			// UGGHHH THIS PROBLEM WILL ALSO OCCUR WITH HOLES 
			// AHHH BUT WE CAN JUST USE AN EPSILON VALUE ...

			// we have one good point here - we just have to choose another value ...
//			delta_x = 	

#if 0
			double v1 = a( x, y);
			double x2 = x + 0.00001f;
			double y2 = y + 0.00001f;
			double v2 = a( x2, y2);
			
			double dist1 = sqrt( pow2( x2 - x) + pow2( y2 - y));
			double dist2 = sqrt( pow2( gx - x) + pow2( gy - y));
			double wx = v1 + (v2 - v1) / ( dist2 - dist1)  ;

			new_model( x, y) = wx ;//mouse_down_model( x, y);
#endif
		}
	}


	//new_model.lock();

	assert( new_model->width());

	return new_model; 

	// right if we record the last model - then what happens if we never updated ? 

/*
	last_model = new_model; 

	command_type cmd( new ContourEditCommand( model, new_model, true));
	command_service.post( cmd);

	// app.log  << "centre v " << g( x1, y1) << std::endl;
	
	log  << "do_stuff " << elapsed_time() << "ms\n";
*/
}






//typedef boost::unordered_map< ptr< IKey> , ptr< IGridEditorJob>, Hash, Pred > objects_t;
typedef std::set<  ptr< IGridEditorJob> > objects_t;

	// the namespace should go over the PositionEditorImpl
	// but it conflicts with the Impl defined in the header.
	// namespace

}; // anon namespace


struct GridEditorInner
{
	GridEditorInner()
		: active( false),
		active_model( )
	{ } 

	bool	active;					// is the controller active

	objects_t	objects; 

	ptr< IGridEditorJob> active_model; 


	ptr< Grid> original_grid; 

	int mouse_down_x; 
	int mouse_down_y; 
};


GridEditor::GridEditor()
	: d( new GridEditorInner )
{ } 

GridEditor::~GridEditor()
{ 
	delete d;
	d = NULL;
} 


void GridEditor:: add(  const ptr< IGridEditorJob> & job )
{
	assert( d->objects.find( job) == d->objects.end() );  
	d->objects.insert( job ); //std::make_pair( key, job ) );
}

void GridEditor::remove( const ptr< IGridEditorJob> & job )
{
	assert( d->objects.find( job) != d->objects.end() );  
	d->objects.erase( job );
}

void GridEditor::set_active( bool active_ )
{
	d->active = active_;
}

/*
	Important. Whether the editor is active or not. could be handled externally. 
	and events delegated/routed depending on this.
*/

	// IMyEvents
void GridEditor::mouse_move( unsigned x, unsigned y ) 
{ 
	if( ! d->active_model )
		return;

	double x1 = d->mouse_down_x; 
	double y1 = d->mouse_down_y;
	double x2 = x;
	double y2 = y;
#if 0
	ptr< IProjection> proj = active_model->get_projection();
	assert( proj);
	agg::trans_affine affine = proj->get_affine();
	affine.invert();
	affine.transform( &x1, &y1  );
	affine.transform( &x2, &y2  );
#endif				
	d->active_model->transform( & x1, & y1 );
	d->active_model->transform( & x2, & y2 );

	ptr< Grid> new_grid = do_stuff( &* d->original_grid, x1, y1, x2, y2 ); 

	d->active_model->set_grid( new_grid );


}

void GridEditor::button_press( unsigned x, unsigned y )
{ 
	if( ! d->active)
		return;

	d->active_model = *d->objects.begin();
	d->active_model->set_active( true );	

	d->original_grid = d->active_model->get_grid();

	// record mouse position	
	d->mouse_down_x = x; 
	d->mouse_down_y = y; 
}

void GridEditor::button_release( unsigned x, unsigned y )
{ 
	if( ! d->active)
		return;

	std::cout << "grid editor button release "  << std::endl;

	// end the transaction 
	if( d->active_model )
	{
//				active_model->finished_edit();	// transaction boundary
//				active_model->set_active( false );	
		// clear

		d->active_model->set_active( false );	
		d->active_model = 0;
	}
}

void GridEditor::key_press( int )
{ }

void GridEditor::key_release( int )
{ }


#if 0
void GridEditor::dispatch( const UIEvent & e)
{
	return;

	//std::cout << "whoot -> Grid Editor got event" << std::endl;
	// like a fsm

	if( ! d->active)
		return;

	switch( e.type)
	{
		case UIEvent::button_press:
		{
			std::cout << "grid editor button press "  << std::endl;
			const UIButtonPress  & event = dynamic_cast< const UIButtonPress & > ( e );  
			assert( ! d->objects.empty());

#if 0
			// we will have to grab the grid object and  project its points to determine hittest.
			foreach( GridEditorImpl::objects_t::value_type & pair , objects )
			{
				IGridEditorJob *object = pair.second;

				// get the projection and the 4 points or create a path and test for inclusion 
				//const agg::trans_affine & affine = object->get_projection()->get_affine();

				object->get_grid()->height() ; 	
				object->get_grid()->width() ; 	
			} 
#endif

			// hittest the model
			// set model active	
			d->active_model = *d->objects.begin();
			d->active_model->set_active( true );	

			d->original_grid = d->active_model->get_grid();

			// record mouse position	
			d->mouse_down_x = event.x; 
			d->mouse_down_y = event.y; 
			break;
		}

		case UIEvent::button_release:
		{
			std::cout << "grid editor button release "  << std::endl;

			// end the transaction 
			if( d->active_model )
			{
//				active_model->finished_edit();	// transaction boundary
//				active_model->set_active( false );	
				// clear

				d->active_model->set_active( false );	
				d->active_model = 0;
			}

			break;	
		}

		case UIEvent::mouse_move: //void mouse_move( int x, int y) 
		{ 
			const UIMouseMove & event = dynamic_cast< const UIMouseMove & >( e );  
			//std::cout << "grid editor mouse move "  << event.x << " " << event.y << std::endl;

			if( ! d->active_model )
				return;

			double x1 = d->mouse_down_x; 
			double y1 = d->mouse_down_y;
			double x2 = event.x;
			double y2 = event.y;
#if 0
			ptr< IProjection> proj = active_model->get_projection();
			assert( proj);
			agg::trans_affine affine = proj->get_affine();
			affine.invert();
			affine.transform( &x1, &y1  );
			affine.transform( &x2, &y2  );
#endif				
			d->active_model->transform( & x1, & y1 );
			d->active_model->transform( & x2, & y2 );

			ptr< Grid> new_grid = do_stuff( &* d->original_grid, x1, y1, x2, y2 ); 

			d->active_model->set_grid( new_grid );

			break;
		} 
	}


}
#endif



//struct GridEditor : IGridEditor
//{

/*
void add_ref()
{
	++count; 
}
void release()
{
	if( --count == 0) 
		delete this; 
}
*/


//private:
//};

//};	// anon namespace


/*

ptr< IGridEditor> create_grid_editor_service()
{
	return new GridEditor;
}
*/

