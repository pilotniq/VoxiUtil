/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
	geometry functions for Voxi
	
*/

#ifndef GEOMETRY_H
#define GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef M_PI
#define M_PI 3.14159265
#endif

typedef unsigned short Angle;

typedef struct sCoordinate
{
	/* x, y, z are position within the room in mm 
		 xzRotation is the angle around the y axis, with 0 being along the z
		 axis. The unit is 1 / 65536'th of a circle */
	int x, y, z;
	Angle xzRotation;
} sCoordinate, *Coordinate;

/* initialization */
void geo_init();

/* trigonometric functions */

__inline short geo_sin( Angle a );
__inline short geo_cos( Angle a );
Angle geo_atan( int dx, int dz );
int geo_distance( Coordinate a, Coordinate b );
		 
Angle geo_directionFromAtoB( Coordinate a, Coordinate b );
/*
	Moves the coordinate <distance> mm in the direction it is pointing 
*/
void geo_moveForward( Coordinate c, int distance );

/*
 * Print a coordinate
 */
void geo_printCoordinate(Coordinate aCoord);

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
                                     Coordinate result);

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
                                    Coordinate result);

/*
 * Translate the first coordinate with the values in the second
 * coordinate. Result coordinate in the third. Rotation is unchanged.
 */
void geo_translateCoordinates(Coordinate objectPos,
                              Coordinate translateBy,
                              Coordinate result);

#ifdef __cplusplus
}
#endif

#endif

