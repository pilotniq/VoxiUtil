/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	Voxi geometry 
*/

#include <math.h>
#include <stdio.h>

#include <voxi/alwaysInclude.h>
#ifdef WIN32
#include <voxi/util/win32_glue.h>
#endif

#include <voxi/util/geometry.h>

CVSID("$Id$");

static int init_count = 0;

static Angle atan_table[ 8193 ];
static short sin_table[ 16385 ];

void geo_init()
{
	int i;
	
  DEBUG("enter\n");

	if( init_count == 0 )
	{
		for( i = 0; i < 8193; i++ )
			atan_table[ i ] = (unsigned short) rint( atan( i / 8192.0 ) * 32768 / M_PI);
		
		for( i = 0; i < 16385; i++ )
    {
      int val = (int) rint( sin( i * (2 * M_PI) / 65536 ) * 32768 );
      sin_table[ i ] = (val >= 32768) ? 32767 : val;
    }
	}
	
	init_count++;

#if 0
  printf("x: %d, z: %d, gatan: %d\n", 0, 50, geo_atan(0,50));
  printf("x: %d, z: %d, gatan: %d\n", 25, 50, geo_atan(25,50));
  printf("x: %d, z: %d, gatan: %d\n", 50, 50, geo_atan(50,50));
  printf("x: %d, z: %d, gatan: %d\n", 50, 25, geo_atan(50,25));

  printf("x: %d, z: %d, gatan: %d\n", 50, 0, geo_atan(50,0));
  printf("x: %d, z: %d, gatan: %d\n", 50, -25, geo_atan(50,-25));
  printf("x: %d, z: %d, gatan: %d\n", 50, -50, geo_atan(50,-50));
  printf("x: %d, z: %d, gatan: %d\n", 25, -50, geo_atan(25,-50));

  printf("x: %d, z: %d, gatan: %d\n", 0, -50, geo_atan(0,-50));
  printf("x: %d, z: %d, gatan: %d\n", -25, -50, geo_atan(-25,-50));
  printf("x: %d, z: %d, gatan: %d\n", -50, -50, geo_atan(-50,-50));
  printf("x: %d, z: %d, gatan: %d\n", -50, -25, geo_atan(-50,-25));

  printf("x: %d, z: %d, gatan: %d\n", -50, 0, geo_atan(-50,0));
  printf("x: %d, z: %d, gatan: %d\n", -50, 25, geo_atan(-50,25));
  printf("x: %d, z: %d, gatan: %d\n", -50, 50, geo_atan(-50,50));
  printf("x: %d, z: %d, gatan: %d\n", -25, 50, geo_atan(-25,50));
#endif

#if 0
  {
    sCoordinate c1 = {300, 50, 300, -8192-16384};
    sCoordinate c2 =  {70, 0, 70, 8192};
    sCoordinate c3;
    
    printf("geocos(0): %d\n", geo_cos(0));
    
    printf("c1: ");
    geo_printCoordinate(&c1);
    printf("\nc2: ");
    geo_printCoordinate(&c2);
    
    geo_transformCoordinatesOutward(&c2, &c1, &c3);
    printf("\nOutward transform: ");
    geo_printCoordinate(&c3);
    

    geo_transformCoordinatesInward(&c3, &c1, &c2);
    printf("\nInward transform: ");
    geo_printCoordinate(&c2);
  }
#endif

  DEBUG("leave\n");

}

/* calculate the Angle in a right-angle triangle where dx is the side opposite
 * to the angle being calculated 
 */
Angle geo_atan( int dx, int dz )
{
	Angle result;

  /*
   * Check for errorcase
   */
  if ((dx == 0) && (dz == 0))
  {
    /* The angle is in fact undefined, but let's return 0 */
    printf("WARNING: geo_atan called with (0,0)...\n");
    return 0;
  }
	
	/* use a table for triangles with angles of less than 45 degrees, and make
		 special cases for the eight possible sections of the circle */
	if( dz > 0 )
		if( dx > 0 )
			if( dz > dx )
				result = atan_table[ dx * 8192 / dz ];
			else
				result = 16384 - atan_table[ dz * 8192 / dx ];
		else
			if( dz > -dx )
				result = 65536 - atan_table[ -dx * 8192 / dz ];
			else
				result = 49152 + atan_table[ dz * 8192 / -dx ];
	else
		if( dx > 0 )
			if( -dz > dx )
				result = 32768 - atan_table[ dx * 8192 / -dz ];
			else
				result = 16384 + atan_table[ -dz * 8192 / dx ];
		else
			if( -dz > -dx )
				result = 32768 + atan_table[ dx * 8192 / dz ];
			else
				result = 49152 - atan_table[ dz * 8192 / dx ];

	/*
	printf( "geo_atan( %d, %d ) = %d\n", dx, dz, result );
	*/
	return result;
}

/* 
	 Return 32768 * sin( a ) 
*/
__inline short geo_sin( Angle a )
{
	if( a < 16384 )
		return sin_table[ a ];
	else if( a < 32768 )
		return sin_table[ 32768 - a ];
	else if( a < 49152 )
		return -sin_table[ a - 32768 ];
	else
		return -sin_table[ 65536 - a ];
}

__inline short geo_cos( Angle a )
{
	if( a < 16384 )
		return sin_table[ 16384 - a ];
	else if( a < 32768 )
		return -sin_table[ a - 16384 ];
	else if( a < 49152 )
		return -sin_table[ 49152 - a ];
	else
		return sin_table[ a - 49152];
}

int geo_distance( Coordinate a, Coordinate b )
{
	int dx, dz;
	
	dx = a->x - b->x;
	dz = a->z - b->z;
	
	/* Long Live Pythagoras! */
	/* WARNING: float function call here! */
	return (int) rint( sqrt( dx * dx + dz * dz ));
}

Angle geo_directionFromAtoB( Coordinate a, Coordinate b )
{
	int dx, dz;
	
	/* change the orientation to face the object we want to look at */
	dx = b->x - a->x;
	dz = b->z - a->z;
	
	return geo_atan( dx, dz );
}

/* Move the coordinate <distance> mm in the orientation of the coordinate */
void geo_moveForward( Coordinate c, int distance )
{
	c->x = c->x + distance * geo_sin( c->xzRotation ) / 32768;
	c->z = c->z + distance * geo_cos( c->xzRotation ) / 32768;
}


/*
 * Print a coordinate
 */
void geo_printCoordinate(Coordinate aCoord)
{
  printf("(x:%d, y:%d, z:%d, xzrot:%d)",
         aCoord->x, aCoord->y, aCoord->z, aCoord->xzRotation);
}

/*
 * Dad's version of what happens:
 *  Transforms the first coordinate out to the same coordinate system
 *  as the second coordinate resides in. That is, the coordinateSystem
 *  is the coordinate system of the objectPos. Result in the third coordinate.
 *
 * Erl's version of what happens:
 *  objectPos is in (relative to) coordinateSystem.
 *  result is relative to the coordinate system that coordinateSystem is
 *    defined in.
 *
 * What is mst's version?
 */
void geo_transformCoordinatesOutward(Coordinate objectPos,
                                     Coordinate coordinateSystem,
                                     Coordinate result)
{
  result->x =
    objectPos->x * geo_cos(coordinateSystem->xzRotation) / 32768 +
    objectPos->z * geo_sin(coordinateSystem->xzRotation) / 32768 +
    coordinateSystem->x;
  result->z =
    - objectPos->x * geo_sin(coordinateSystem->xzRotation) / 32768 +
    objectPos->z * geo_cos(coordinateSystem->xzRotation) / 32768 +
    coordinateSystem->z;
  result->y = objectPos->y + coordinateSystem->y;
  result->xzRotation =
    objectPos->xzRotation +
    coordinateSystem->xzRotation;
}


/*
 * Dad's version of what happens:
 *  Transforms the first coordinate in to the coordinate system
 *  specified by coordinateSystem. The pos and coordinateSystem
 *  reside in the same coordinate system. Result is the position
 *  of objectPos in the coordinate system coordinateSystem.
 *
 * Erl's version of what happens:
 *  objectPos is defined in the same coordinate system that
 *  coordinateSystem is defined in.
 *  result is objectPos relative to coordinateSystem.
 *
 * What is mst's version?
 */
void geo_transformCoordinatesInward(Coordinate objectPos,
                                    Coordinate coordinateSystem,
                                    Coordinate result)
{
  int tempx, tempz;
  tempx = objectPos->x - coordinateSystem->x;
  tempz = objectPos->z - coordinateSystem->z;
  
  result->x =
    tempx * geo_cos( (unsigned short) -coordinateSystem->xzRotation) / 32768 +
    tempz * geo_sin( (unsigned short) -coordinateSystem->xzRotation) / 32768;
  
  result->z =
    - tempx * geo_sin( (unsigned short) -coordinateSystem->xzRotation) / 32768 +
    tempz * geo_cos( (unsigned short) -coordinateSystem->xzRotation) / 32768;

  result->y = objectPos->y - coordinateSystem->y;
  result->xzRotation =
    objectPos->xzRotation -
    coordinateSystem->xzRotation;
}



/*
 * Translate the first coordinate with the values in the second
 * coordinate. Result coordinate in the third. The rotation is unchanged.
 */
void geo_translateCoordinates(Coordinate objectPos,
                              Coordinate translateBy,
                              Coordinate result)
{
  result->x = objectPos->x + translateBy->x;
  result->y = objectPos->y + translateBy->y;
  result->z = objectPos->z + translateBy->z;
  result->xzRotation = objectPos->xzRotation;

}

