
#pragma once

/*
	this class is used, because agg_path_storage includes and iterator. therefore even if we have a basically
	const operation (just iterating the vertices using), it means we cannot pass agg_path_storage with const .
	
	therefore use, this class as an adaptor to a const path_storage
	
	it also makes the dependencies of operations, like geom transform or rendering clearer.
*/

#include <agg_path_storage.h>


template< class VC>
struct path_reader_base 
{ 
    typedef VC		container_type;

	const container_type	 & m_vertices;
    unsigned        m_iterator;

	path_reader_base( const agg::path_storage & path )
		: m_vertices( path.vertices() )
	{
		rewind( 0);
	}	

	path_reader_base( const container_type	 & m_vertices)
		: m_vertices( m_vertices)
	{ 
		rewind( 0);
	} 

    inline void rewind(unsigned path_id) 
    {
        m_iterator = path_id;
    }

    //------------------------------------------------------------------------
    inline unsigned vertex(double* x, double* y) 
    {
        if(m_iterator >= m_vertices.total_vertices()) return agg::path_cmd_stop;
        return m_vertices.vertex(m_iterator++, x, y);
    }
};

typedef path_reader_base< agg::vertex_block_storage<double> > path_reader; 


