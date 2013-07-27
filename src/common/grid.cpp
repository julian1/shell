
#include <common/grid.h>

#include <cmath>	// exp,  rad



static double test_function( double i, double j)
{
	// nice test function
	// expects values in approx range 0-1
	
	// example taken from somewhere on web 
	double x, y, rad;
    double h1, h2, h3;
    h1 = 0.5*0.5;
    h2 = 0.75*0.75;
    h3 = 0.25*0.25;
                 
	y = 2.0*(double)j  - 1.0;          
	x = 2.0*(double)i  - 1.0;
	rad = (x-0.5)*(x-0.5) + (y+0.5)*(y+0.5);
	double f = exp( -rad/h1 );
	rad = (x+0.3)*(x+0.3) + (y-0.75)*(y-0.75);
	f += exp( -rad/h2 );
	rad = (x+0.7)*(x+0.7) + (y+0.6)*(y+0.6);
	f += exp( -rad/h3 );
	return f;
};


ptr< Grid>  make_test_grid( unsigned w, unsigned h)
{
	// if we wrap in an intrusive ptr then it will get deleted before the function exits 
//	ptr< Grid> m = new Grid( w, h);
	ptr< Grid>  m( new Grid( w, h) );

	for( unsigned y = 0; y < m->height(); y++)
	{
		for( unsigned x = 0; x < m->width(); x++) 
		{ 
			double i = x / double( m->width() );		// -1 ?
			double j = y / double( m->height() );			

			(*m)( x, y) = test_function( i, j);
		}
	}

	return m;
}



