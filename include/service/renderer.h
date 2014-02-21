
#pragma once

#include <common/ptr.h> 
#include <vector> 

/*
	- if we got rid of the IKey. They we could do straight pointer comparisons on the interface, 
	for equality, and the hashing could be done on the pointer value.

	- perhaps the IKey, simplifies some things when we combine an aggregate and proj. but for
	adaptors, it may be better to remove.  
*/

struct BitmapSurface;

struct Rect
{
	Rect( int x, int y, int w, int h )
		: x( x), y( y), w( w), h( h )
	{ } 
	int x, y;
	int w, h;
};


struct RenderParams
{
	RenderParams( BitmapSurface & surface, int dt )
		: surface( surface),
		dt( dt)
	{ } 

	int dt; 
	BitmapSurface & surface;
};


struct IRenderJob
{
	// called before all render calls.
	// VERY Ihooked MPORTANT - use this for co-ordianting cross layer actions like label positioning  
	// this will most likely be used by a service 
	virtual void pre_render( RenderParams & render_params) = 0;

	virtual void render( RenderParams & render_params) = 0 ; 
	// virtual void render_ps ( std::ostream & surface ) = 0 ; 

	// change name get_render_z_order()
	virtual int get_z_order() const = 0; 

	// might actually want to be a vector< Rect> that gets populated, which would allow for multiple items 
	// or else a referse interface
	virtual void get_bounds( int *x1, int *y1, int *x2, int *y2 ) = 0;  
};



struct IRenderer 
{
	virtual void add( IRenderJob & job ) = 0; 
	virtual void remove( IRenderJob & job ) = 0; 
	virtual void notify( IRenderJob & job ) = 0; 

	virtual void resize( int x, int y ) = 0; 

	// update is now a sequence
	// return the list of regions that must be updated (they may overlap)
	// and set some state for update2
	virtual void render_and_invalidate( std::vector< Rect> & regions ) = 0;  

	// return a surface, that is sufficient to cover the invalid regions. the passed regions are a superset of regions in update1
	virtual ptr< BitmapSurface> update_expose( const std::vector< Rect> & regions ) = 0;  
};

// this definition shouldn't be here...
// should only be a struct

struct IRenderControl;
struct Timer;

struct Renderer  : IRenderer
{
	Renderer( IRenderControl & render_control, Timer & timer  );
	~Renderer();

	void add( IRenderJob & job ) ;
	void remove( IRenderJob & job ); 
	void notify( IRenderJob & job );  

	void resize( int x, int y );

	// Rather than returning a set of regions, why don't we pass an interface for what needs
	// to be invalidated ...

	void render_and_invalidate( std::vector< Rect> & regions );

	// return a surface, that is sufficient to cover the invalid regions. the passed regions are a superset of regions in update1
	ptr< BitmapSurface> update_expose( const std::vector< Rect> & regions );

private:
	struct Inner *d;
	Renderer( const Renderer & );
	Renderer & operator = ( const Renderer & );
};


