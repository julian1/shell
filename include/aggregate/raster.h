/*
	lets try the macro thing with the aggregate.
*/
/*
	Jobs that are not update/tracking  like loading from files should not be placed in the subsystem/service

		job.play( new LoadShapesFile( "shapefile.shp", required_projection ) ); 
		job.play( new LoadTestAggGrid( required_projection ) ); 

	the point about these commands is that they want to be easily instantiated anywhere.
	- they are complicated tasks, deserving abstraction, but not 

	services that require update (eg. contouring, rendering, projection clipping) are different.
		The commander would call the command passing back the services etc.

	they are basically like functions. but there is no obvious place to put them. 
*/
/*
	Ok, I am 
	We have problems. 
	Are we going to make the loader independent. 

	- There should be an adaptor/command to load the aggregate which is mostly fucking state.
	- It should hopefully also be able to be shared.
*/

/*
	OK.
		Why not make loading a fucking command. Why should it be pushed into a service/controller.
		The loader doesn't have to even know.

		persisting it out to a controller is silly, when a simple command playing service will do.

		all the shapes service should be got rid off. and replaced with a load command structure. 
		cqrs.
		An adaptor is like a command. except it is persisted in the service/controller, while the 
		comand is persisted in the play. 

		Commander.play( new LoadShapesFile( "shapefile.shp", required_projection ) ); 
		The commander would then take the services etc as constructor references in order to do it's job.
		The Commands have nothing to do with the event store. (But we could do tricky things if required).
*/

/*
	The same should be done for the contour grid. and the shapes loading.
*/

//////////////////////////////////
/*
	GOALS,
		(1) lets keep the projection out (to enable multiple views), 
		(2) and try to use commands to load
		(3) use virtual interface rather than self pointer. 
*/
#include <common/ptr.h>
#include <common/bitmap.h>

#include <agg_trans_affine.h>
#include <agg_path_storage.h>

struct IRasterAggregateRoot
{
	virtual void add_ref() = 0;
	virtual void release() = 0;

	virtual ptr< Bitmap> get_surface() const = 0; 
	virtual void set_surface( const ptr< Bitmap> &  ) = 0; 

	virtual void set_geo_reference( const agg::trans_affine & ) = 0; 
};

struct Services;
struct IProjection;

struct IProjectionAggregateRoot;

ptr< IRasterAggregateRoot> create_test_raster_aggregate_root();	// create T

void add_raster_aggregate_root( Services & services, const ptr< IRasterAggregateRoot> & root , const ptr< IProjectionAggregateRoot> & proj_aggregate ); 
void remove_raster_aggregate_root( Services & services, const ptr< IRasterAggregateRoot> & root, const ptr< IProjectionAggregateRoot> & projection_aggregate  );



