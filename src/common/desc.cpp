
#include <common/desc.h>

bool operator == ( const Level & a, const Level & b )
{
	if( a.type != b.type )
		return false;

	switch( a.type )
	{
		// it would be neater if this stuff was Level::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::level_surface: 
		case Desc::level_max_wind_speed:
		case Desc::level_tropopause:
			return true;
		case Desc::level_isobaric:
			return ((LevelIsobaric & )a).hpa == ((LevelIsobaric & )b).hpa ; 

		case Desc::level_isobaric_range:
			return ((LevelIsobaricRange & )a).hpa1 == ((LevelIsobaricRange & )b).hpa1
				&& ((LevelIsobaricRange & )a).hpa2 == ((LevelIsobaricRange & )b).hpa2 ;
		default:
			std::cout << "a.type " << a.type << std::endl;
			assert( 0);
	};
	assert( 0);
}


std::ostream & operator << ( std::ostream & os, const Level & level ) 
{
	switch( level.type )
	{
		case Desc::level_surface: 
			os << "surface"; 
			break;
		case Desc::level_max_wind_speed:
			os << "max wind"; 
			break;
		case Desc::level_tropopause:
			os << "tropopause"; 
			break;
		case Desc::level_isobaric:
			os << ((LevelIsobaric & )level).hpa << "hpa"; 
			break;
		case Desc::level_isobaric_range:
			os << ((LevelIsobaricRange & )level).hpa1 
			<< "-" << ((LevelIsobaricRange & )level).hpa2 << "hpa"; 
			break;
		default:
			std::cout << "type " << level.type << std::endl;
			assert( 0);
	};
	return os;
}




bool operator == ( const Valid & a, const Valid & b )
{
	if( a.type != b.type )
		return false;

	switch( a.type )
	{
		// it would be neater if this stuff was Valid::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::valid_simple:
			return ((ValidSimple & )a).valid == ((ValidSimple & )b).valid ; 

		case Desc::valid_range:
			return ((ValidRange & )a).start == ((ValidRange & )b).start 
				&& ((ValidRange & )a).finish == ((ValidRange & )b).finish ;
		default:
			std::cout << "a.type " << a.type << std::endl;
			assert( 0);
	};
	assert( 0);
}

	
std::ostream & operator << ( std::ostream & os, const Valid & valid ) 
{
	switch( valid.type )
	{
		case Desc::valid_simple:
			os << ((ValidSimple &)valid ). valid ; 
			break;
		case Desc::valid_range:
			os << ((ValidRange &)valid ). start << "-" << ((ValidRange &)valid ). finish ; 
			break;
		default:
			assert( 0 );
	}
	return os;
}	


bool operator == ( const Param & a, const Param & b )
{
	if( a.type != b.type )
		return false;

	switch( a.type )
	{
		// it would be neater if this stuff was Param::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::param_temp :
			// do we match on the unit ???? 
		case Desc::param_relative_humidity : 
		case Desc::param_geopot_height : 
		case Desc::param_wind: 
			return true;
		case Desc::param_generic_grib: 
			return ((ParamGenericGrib & )a).code == ((ParamGenericGrib & )b).code ; 
		default:
			std::cout << "a.type " << a.type << std::endl;
			assert( 0);
	};
	assert( 0);
}


std::ostream & operator << ( std::ostream & os, const Param & param ) 
{
	switch( param.type )
	{
		// it would be neater if this stuff was Param::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::param_temp :
			// do we match on the unit ???? 
			os << "temp (unit?)";
			break;
		case Desc::param_relative_humidity : 
			os << "relative humidity";
			break;
		case Desc::param_geopot_height : 
			os << "geopot height";
			break;
		case Desc::param_wind: 
			os << "wind";
			break;
		case Desc::param_generic_grib: 
			os << "generic grib " << ((ParamGenericGrib & ) param).code ;
			break;
		default:
			std::cout << "a.type " << param.type << std::endl;
			assert( 0);
	};
	return os;
}




bool operator == ( const Area & a, const Area & b )
{
	if( a.type != b.type )
		return false;

	switch( a.type )
	{
		// it would be neater if this stuff was Area::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::area_ll_grid:
			return 
				((AreaLLGrid &) a).top == ((AreaLLGrid &) b).top  
				&& ((AreaLLGrid &) a).bottom == ((AreaLLGrid &) b).bottom  
				&& ((AreaLLGrid &) a).left == ((AreaLLGrid &) b).left  
				&& ((AreaLLGrid &) a).right == ((AreaLLGrid &) b).right ; 
		default:
			assert( 0);
	}
}


std::ostream & operator << ( std::ostream & os, const Area & area ) 
{
	switch( area.type )
	{
		// it would be neater if this stuff was Area::surface rather  than Desc::surface   ...
		// and we were independent

		case Desc::area_ll_grid:
			os  
				<< ((AreaLLGrid &) area).top 
				<< " " << ((AreaLLGrid &) area).bottom 
				<< " " << ((AreaLLGrid &) area).left 
				<< " " << ((AreaLLGrid &) area).right 
			; 
			break;
		default:
			assert( 0);
	}
	return os;
}




bool operator == ( const SurfaceCriteria & a, const SurfaceCriteria & b ) 
{
	return *a.param == *b.param 
		&& *a.level == *b.level
		&& *a.valid == *b.valid
		&& *a.area == *b.area ; 
}


