
#pragma once

#include <common/events.h>
#include <vector>

/*
	- if we got rid of the IKey. They we could do straight pointer comparisons on the interface,
	for equality, and the hashing could be done on the pointer value.

	- perhaps the IKey, simplifies some things when we combine an aggregate and proj. but for
	adaptors, it may be better to remove.
*/
//struct INotify; 
struct Bitmap;

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
	RenderParams( Bitmap & surface, int dt )
		: surface( surface),
		dt( dt)
	{ }

	int dt;
	Bitmap & surface;
};


struct INotify; 

struct IRenderJob : IObject
{
	// called before all render calls.
	// VERY Ihooked MPORTANT - use this for co-ordianting cross layer actions like label positioning
	// this will most likely be used by a service
	virtual void pre_render( RenderParams & params) = 0;

	virtual void render( RenderParams & params) = 0 ;
	// virtual void render_ps ( std::ostream & surface ) = 0 ;

	// change name get_render_z_order()
	virtual int get_z_order() const = 0;

	// might actually want to be a vector< Rect> that gets populated, which would allow for multiple items
	// or else a referse interface
	virtual void get_bounds( int *x1, int *y1, int *x2, int *y2 ) = 0;
};



struct IRenderer : virtual IObject
{
	virtual void register_( INotify * l) = 0;
	virtual void unregister( INotify * l) = 0;

	// render jobs
	virtual void add( IRenderJob & job ) = 0;
	virtual void remove( IRenderJob & job ) = 0;

	virtual void resize( int w, int h ) = 0;
	virtual void getsize( int * w, int * h ) = 0;

	// render and return the minimum list of regions that must be updated
	virtual void render( std::vector< Rect> & regions ) = 0;

	// return a surface, that is sufficient to cover the invalid regions. the passed regions are a superset of regions in update1
	virtual void update_expose( const std::vector< Rect> & regions, Bitmap & result ) = 0;
};


struct Renderer  : IRenderer
{
	Renderer();
	~Renderer();

	void register_( INotify * l);
	void unregister( INotify * l);
	void notify( const char *msg); 

	void add( IRenderJob & job ) ;
	void remove( IRenderJob & job );

	void on_job_changed( const Event &e ); 

	void resize( int x, int y );
	void getsize( int * w, int * h ) ;

	void render( std::vector< Rect> & regions );
	void  update_expose( const std::vector< Rect> & regions, Bitmap & result );

private:
	struct Inner *d;
	Renderer( const Renderer & );
	Renderer & operator = ( const Renderer & );
};


