
#pragma once

/*
	- We could avoid having to maintain the service reference in the Application class, 
	if we made these reference use ptr<> references. 

	- It avoids having to have to everthing maintained twice. 
*/

/*
	Loading things, like shapes from shapefile, or png does not need a service. 
	they are atomic procedures. actually the ability to introduce seams could

	The reason we might want to make them services, 
		(1) is to persist other state. like default directories for file lookup. 
		(2) to be able to instantiate different versions (win32 font load/ freetype)
		(3) optimisation (cahcing of font service).

	But in these cases the client doesn't register a full interface with 
	the service using add/remove.  
	
	The registering (for rendering/grid_edit) is a fundamentally different thing
	they are controllers and views. 
*/


/*
	OK, I think these need to be split between updating services and non-updating services.
*/
//#include <common/ptr.h>

struct IRenderer;
//struct IProjector;	
struct IGridEditor;	
struct IPositionEditor;
struct IFonts;
struct ILabels;
struct ILayers;

//struct IGribsService; 
struct ILevelController; 
struct IValidController ; 

// Suspect the Projector should potentially be removed. Not a cross layer operation, 
// if have multiple jobs, can schedule // except raster. 

// ok, the problem is that the to use ptr we will need the full declaration ...

struct Services
{
	Services( 
		// primary view stuff
		ILayers		& layers , 
		ILabels		& labels ,
		IRenderer	& renderer,

		// secondary controllers / services
		IGridEditor	& grid_editor,
		IPositionEditor	& position_editor,
		IFonts		& fonts ,
//		IGribsService & gribs_service,
		ILevelController & level_controller,
		IValidController  & valid_controller
	)
	:
		layers( layers ), 
		labels( labels ),
		renderer( renderer), 

		grid_editor( grid_editor ),
		position_editor( position_editor ),
		fonts( fonts),
//		gribs_service( gribs_service ),
		level_controller( level_controller),
		valid_controller( valid_controller )
	{ } 
	
	ILayers		& layers;
	ILabels		& labels; 
	IRenderer	& renderer; 


	IGridEditor	& grid_editor;
	IPositionEditor	& position_editor;
	IFonts		& fonts ; 
//	IGribsService & gribs_service;
	ILevelController & level_controller; 
	IValidController  & valid_controller; 
};



