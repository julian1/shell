
#include <common/load_shapefile.h> 

// can we reference this some other way ?
#include "shapefil.h"


#include <cassert>
#include <cstdlib>	//exit uggh

//#include <vector>
//#include <string>


void load_shapes( const std::string & filename_, IShapeLoadCallback & callback  )
/*
int load_shapes( 
	const std::string & filename_, 

	std::vector< ptr< Shape > >	& shapes
//	std::vector< agg::path_storage > &shapes 
) */
//int load_shapes( const char *filename, agg::path_storage &path)
{
//	assert( shapes.empty() );

	const char *filename = filename_.c_str();

    SHPHandle	hSHP;
    int		nShapeType, nEntities, i, iPart, bValidate = 0,nInvalidCount=0;
    const char 	*pszPlus;
    double 	adfMinBound[4], adfMaxBound[4];

       bValidate = 1;
/*
    if( argc > 1 && strcmp(argv[1],"-validate") == 0 )
    {
        bValidate = 1;
        argv++;
        argc--;
    }
*/

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
/*    if( argc != 2 )
    {
	printf( "shpdump [-validate] shp_file\n" );
	exit( 1 );
    }
*/
/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */

//	const char *filename = "world_shape/world_adm0.shp"; 
///	const char *filename = "world_adm0.shp"; 

//	fprintf( stdout, "trying to open '%s'\n", filename );

    hSHP = SHPOpen( /*argv[1]*/ filename, "rb" );

    if( hSHP == NULL )
    {
		printf( "Unable to open:%s\n", filename );
		exit( 1 );
    }




/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    SHPGetInfo( hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

#if 0
    printf( "Shapefile Type: %s   # of Shapes: %d\n\n",
            SHPTypeName( nShapeType ), nEntities );
    
    printf( "File Bounds: (%12.3f,%12.3f,%g,%g)\n"
            "         to  (%12.3f,%12.3f,%g,%g)\n",
            adfMinBound[0], 
            adfMinBound[1], 
            adfMinBound[2], 
            adfMinBound[3], 
            adfMaxBound[0], 
            adfMaxBound[1], 
            adfMaxBound[2], 
            adfMaxBound[3] );
#endif   

/*
	ok - rather than have an array of paths for each shape we include the meta data
	we should just keep the paths

*/

 
/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
		agg::path_storage	path;
	//	ptr< Shape>		shape = new Shape; 
	//	shapes.push_back( agg::path_storage() );

		int		j;
        SHPObject	*psShape;

		psShape = SHPReadObject( hSHP, i );

#if 0
		printf( "\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
			"  Bounds:(%12.3f,%12.3f, %g, %g)\n"
			"      to (%12.3f,%12.3f, %g, %g)\n",
			i, SHPTypeName(psShape->nSHPType),
			psShape->nVertices, psShape->nParts,
			psShape->dfXMin, psShape->dfYMin,
			psShape->dfZMin, psShape->dfMMin,
			psShape->dfXMax, psShape->dfYMax,
			psShape->dfZMax, psShape->dfMMax );

		printf( "\n");
#endif
		for( j = 0, iPart = 1; j < psShape->nVertices; j++ )
		{
			assert( iPart <= psShape->nParts);
			
			if( j == 0  )
			{
				// definitions in  shapefil.h
				assert( psShape->panPartType[ 0] == SHPP_RING );
				// printf( "////  type %s\n", SHPPartTypeName( psShape->panPartType[0] ));
				//printf( "* move_to (1)  %12.3f %12.3f\n", psShape->padfX[j], psShape->padfY[j]);
				path.move_to( psShape->padfX[j], psShape->padfY[j]);
			}
			
			// last item in a poly is close - gets last item ( note --- we dont want to add it again )  
			else if( j == psShape->nVertices - 1 )
			{
				// printf( "* close poly (1) (ignore %12.3f %12.3f)\n", psShape->padfX[j], psShape->padfY[j]);
				path.close_polygon();
			}
			else if( (	iPart < psShape->nParts && psShape->panPartStart[ iPart] - 1 == j ))
			{
				// printf( "* close poly (2) (ignore %12.3f %12.3f)\n", psShape->padfX[j], psShape->padfY[j]);
				path.close_polygon();
			} 
			else if( (	iPart < psShape->nParts && psShape->panPartStart[ iPart]  == j ))
			{
				// only point where we increment iPart
				iPart++;
				//printf( "* move_to (2) %12.3f %12.3f\n", psShape->padfX[j], psShape->padfY[j]);
				path.move_to( psShape->padfX[j], psShape->padfY[j]);
			} 

			else {
				// printf( "* line_to  %12.3f %12.3f\n", psShape->padfX[j], psShape->padfY[j]);
				path.line_to( psShape->padfX[j], psShape->padfY[j]);
			}
			// the values seem to indicate that they are closed ...
			/*
				printf("%d   %s (%12.3f,%12.3f, %g, %g) %s \n",
					iPart, 
				   pszPlus,
				   psShape->padfX[j],
				   psShape->padfY[j],
				   psShape->padfZ[j],
				   psShape->padfM[j],
				   pszPartType );
			*/
		}

		if( bValidate )
		{
			int nAltered = SHPRewindObject( hSHP, psShape );

			if( nAltered > 0 )
			{
				printf( "  %d rings wound in the wrong direction.\n", nAltered );
				nInvalidCount++;
				exit( 0); 
			}
		}
        SHPDestroyObject( psShape );

		callback.add_shape( i, path );
//		shapes.push_back( shape);
    }

    SHPClose( hSHP );

    if( bValidate && nInvalidCount )
    {
        printf( "%d object has invalid ring orderings.\n", nInvalidCount );
    }
#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif
//    exit( 0 );
}



#include <iostream>
#include <vector>



int load_shape_attributes( const std::string & filename, IShapeAttributeLoadCallback & callback ) 
{
	std::vector< std::string>	fields;   

    DBFHandle	hDBF;
    int		*panWidth, i, iRecord;
    char	szFormat[32];

	const char * pszFilename = NULL;

    int		nWidth, nDecimals;
//    int		bHeader = 0;
//    int		bRaw = 0;
//    int		bMultiLine = 0;
    char	szTitle[12];

//	fprintf( stdout, "load_shape_attributes\n" );
//	fflush( stdout);


/* -------------------------------------------------------------------- */
/*      Handle arguments.                                               */
/* -------------------------------------------------------------------- */

//	bMultiLine = 1;
#if 0
    for( i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i],"-h") == 0 )
            bHeader = 1;
        else if( strcmp(argv[i],"-r") == 0 )
            bRaw = 1;
        else if( strcmp(argv[i],"-m") == 0 )
            bMultiLine = 1;
        else
            pszFilename = argv[i];
    }
#endif

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */



     pszFilename = filename.c_str(); 
#if 0
    if( pszFilename == NULL )
    {
	printf( "dbfdump [-h] [-r] [-m] xbase_file\n" );
        printf( "        -h: Write header info (field descriptions)\n" );
        printf( "        -r: Write raw field info, numeric values not reformatted\n" );
        printf( "        -m: Multiline, one line per field.\n" );
	exit( 1 );
    }
#endif
/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    hDBF = DBFOpen( pszFilename, "rb" );
    if( hDBF == NULL )
    {
	printf( "DBFOpen(\"r\") failed.\n" );
	exit( 2 );
    }
	else
	{
	//	fprintf( stdout, "opened dbf file\n"  ); 
	} 
/* -------------------------------------------------------------------- */
/*	If there is no data in this file let the user know.		*/
/* -------------------------------------------------------------------- */
    if( DBFGetFieldCount(hDBF) == 0 )
    {
		printf( "There are no fields in this table!\n" );
		exit( 3 );
    }

/* -------------------------------------------------------------------- */
/*	Dump header definitions.					*/
/* -------------------------------------------------------------------- */

#if 0
    if( bHeader )
    {
        for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
        {
            DBFFieldType	eType;
            const char	 	*pszTypeName;

            eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
            if( eType == FTString )
                pszTypeName = "String";
            else if( eType == FTInteger )
                pszTypeName = "Integer";
            else if( eType == FTDouble )
                pszTypeName = "Double";
            else if( eType == FTInvalid )
                pszTypeName = "Invalid";

            printf( "Field %d: Type=%s, Title=`%s', Width=%d, Decimals=%d\n",
                    i, pszTypeName, szTitle, nWidth, nDecimals );
        }
    }
#endif
/* -------------------------------------------------------------------- */
/*	Compute offsets to use when printing each of the field 		*/
/*	values. We make each field as wide as the field title+1, or 	*/
/*	the field value + 1. 						*/
/* -------------------------------------------------------------------- */

#if 0
    panWidth = (int *) malloc( DBFGetFieldCount( hDBF ) * sizeof(int) );

    for( i = 0; i < DBFGetFieldCount(hDBF) && !bMultiLine; i++ )
    {
	DBFFieldType	eType;

	eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
	if( strlen(szTitle) > nWidth )
	    panWidth[i] = strlen(szTitle);
	else
	    panWidth[i] = nWidth;

	if( eType == FTString )
	    sprintf( szFormat, "%%-%ds ", panWidth[i] );
	else
	    sprintf( szFormat, "%%%ds ", panWidth[i] );
	printf( szFormat, szTitle );
    }
    printf( "\n" );

#endif

/* -------------------------------------------------------------------- */
/*	Read all the records 						*/
/* -------------------------------------------------------------------- */

	// loop the field names       
	for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
	{
		DBFFieldType	eType;
		eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
	//	 printf( "name %s: ", szTitle );

		fields.push_back( szTitle  ) ; 
	}	

 

    for( iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++ )
    {

//		std::cout << "iRecord " << iRecord << std::endl;

		//records.push_back( std::vector< std::string> ()  ) ; 

//        if( bMultiLine )
       
//		 printf( "Record: %d\n", iRecord );
 
		// loop the fields       
		for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
		{
			DBFFieldType	eType;
            
            eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );

        //      printf( "name %s: ", szTitle );

 //           if( bMultiLine )
  //          {
   //         }
            
/* -------------------------------------------------------------------- */
/*      Print the record according to the type and formatting           */
/*      information implicit in the DBF field description.              */
/* -------------------------------------------------------------------- */
            //if( !bRaw )

			if( DBFIsAttributeNULL( hDBF, iRecord, i ) )
			{

				//	callback.add_attr( iRecord, fields.at( i), (void *)0 );  

			}
			else
			{
				switch( eType )
				{
					case FTString:
					{
						// sprintf( szFormat, "string %%-%ds", nWidth );

						const char *value  =  DBFReadStringAttribute( hDBF, iRecord, i );
						//fprintf( stdout, "str '%s'", s );
						//printf( szFormat, DBFReadStringAttribute( hDBF, iRecord, i ) );
	
						// do we have to free the memory....
			
						callback.add_attr( iRecord, fields.at( i), std::string( value ));  
						break;
					}
					case FTInteger:
					{
					/*	
						sprintf( szFormat, "integer %%%dd", nWidth );
						printf( szFormat, DBFReadIntegerAttribute( hDBF, iRecord, i ) );
					*/
						int value = DBFReadIntegerAttribute( hDBF, iRecord, i ); 
						callback.add_attr( iRecord, fields.at( i), value );  
						break;
					}
					case FTDouble:
					{
						/*
						sprintf( szFormat, "double %%%d.%dlf", nWidth, nDecimals );
						printf( szFormat, DBFReadDoubleAttribute( hDBF, iRecord, i ) );
						*/

						double value = DBFReadDoubleAttribute( hDBF, iRecord, i ); 
						callback.add_attr( iRecord, fields.at( i), value );  
						break;
					}	
					 default:
						assert( 0);
					break;
				}
			}
            

/* -------------------------------------------------------------------- */
/*      Just dump in raw form (as formatted in the file).               */
/* -------------------------------------------------------------------- */
#if 0
            else
            {
                sprintf( szFormat, "%%-%ds", nWidth );
                printf( szFormat, 
                        DBFReadStringAttribute( hDBF, iRecord, i ) );
            }
#endif
/* -------------------------------------------------------------------- */
/*      Write out any extra spaces required to pad out the field        */
/*      width.                                                          */
/* -------------------------------------------------------------------- */
#if 0
	    if( !bMultiLine )
	    {
		sprintf( szFormat, "%%%ds", panWidth[i] - nWidth + 1 );
		printf( szFormat, "" );
	    }

            if( bMultiLine )
#endif

#if 0
		printf( "\n" );
	    fflush( stdout );
#endif
		}
//	printf( "\n" );
    }

	//std::cout << "done loading attributes !!! " << std::endl;

    DBFClose( hDBF );

    return( 0 );
}

