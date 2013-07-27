
#pragma once

#include <vector> 

/*
	- if we got rid of the IKey. They we could do straight pointer comparisons on the interface, 
	for equality, and the hashing could be done on the pointer value.

	- perhaps the IKey, simplifies some things when we combine an aggregate and proj. but for
	adaptors, it may be better to remove.  
*/

struct IKey;
struct BitmapSurface;
struct UpdateParms;



struct Rect
{
	Rect( int x, int y, int w, int h )
		: x( x), y( y), w( w), h( h )
	{ } 
	int x, y;
	int w, h;
};



struct IRenderJob
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual void render ( BitmapSurface & surface, const UpdateParms & parms ) = 0 ; 

	// virtual void render_ps ( std::ostream & surface, const UpdateParms & parms ) = 0 ; 

	// change name get_render_z_order(), to distinguish from label_z_order ? 
	virtual int get_z_order() const = 0; 

	// no change in z_order, but mandates that there is a one-off change for update round
	virtual bool get_invalid() const = 0;		

	// might actually want to be a vector< Rect> that gets populated, which would allow for multiple items 
	// or else a referse interface
	virtual void get_bounds( double *x1, double *y1, double *x2, double *y2 ) = 0;  
};



struct IRenderer 
{

	virtual void add( const ptr< IRenderJob> & job ) = 0; 
	virtual void remove( const ptr< IRenderJob> & job ) = 0; 

//	virtual void add( const ptr< IKey> & key, const ptr< IRenderJob> & job ) = 0; 
//	virtual void remove( const ptr< IKey> & key ) = 0; 

	virtual void resize( int x, int y ) = 0; 

	// update is now a sequence
	// return the list of regions that must be updated (they may overlap)
	// and set some state for update2
	virtual void update_render( const UpdateParms & parms, std::vector< Rect> & regions ) = 0;  

	// return a surface, that is sufficient to cover the invalid regions. the passed regions are a superset of regions in update1
	virtual ptr< BitmapSurface> update_expose( const std::vector< Rect> & regions ) = 0;  
	//	virtual void update( const UpdateParms & parms ) = 0; 
};


struct Renderer  : IRenderer
{
	Renderer();
	~Renderer();

	void add( const ptr< IRenderJob> & job ) ; 
	void remove( const ptr< IRenderJob> & job ) ; 

//	void add( const ptr< IKey> & key, const ptr< IRenderJob> & job );
//	void remove( const ptr< IKey> & key );

	void resize( int x, int y );

	void update_render( const UpdateParms & parms, std::vector< Rect> & regions );

	// return a surface, that is sufficient to cover the invalid regions. the passed regions are a superset of regions in update1
	ptr< BitmapSurface> update_expose( const std::vector< Rect> & regions );
private:
	struct Inner *d;
	Renderer( const Renderer & );
	Renderer & operator = ( const Renderer & );
};



/*
	it would be ok, to inject gtk stuff if really 
*/


// ptr< IRenderer>	create_renderer_service( );

/*
struct IBlit 
{
	// the surface is the full screen, and we just want to blit the area
	// given by 
	virtual void blit( int x, int y, int w, int h, const BitmapSurface & surface ) = 0; 
};
*/

/*
struct IInvalidate
{
	virtual void invalidate( int x, int y, int w, int h ) = 0; 
};
*/

// ok, the alternative to having to sequence the rendering over two calls, would be to 
// inject the gtk drawing area, and the  

// 



//ptr< IRenderer>	create_better_renderer_service( BitmapSurface & surface /* gtk */  );


	// renderer calls this, when the update occurs. 
	// so we could actually pass as a UpdateParms argument, not in the constructor. 
	// then the server will come back with a bunch of areas to update. 


/*
	the sequence is that update occurs, and it provides a list of invalid areas.
	these are then sent to the server. 
	then an event occurs asking for repaint.

	Note, that the server could ask to repaint any area.
*/

// should we just inject the drawing_area in, then we could hook it's events  ???
// no, becaues there are other events like resize(), that require more handling logic
// being able to abstract
// perhaps the 








