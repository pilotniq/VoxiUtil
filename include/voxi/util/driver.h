/*
 * driver.h
 *
 * Helper code for drivers - that is code in a shared library/DLL loaded at 
 * runtime, containing functions with known names, which are stored as
 * function pointers in a driver struct
 */

#include <stddef.h> /* size_t defined here */
#include <voxi/util/err.h>
#include <voxi/util/shlib.h>

/*
 *  Type definitions
 */
typedef struct sDriverFunctionInfo
{
  const char *name;
  size_t offset;
} sDriverFunctionInfo, *DriverFunctionInfo;

typedef Error (*DriverGetAPIVersionFuncPtr)( int *version );

/*
  Error codes
  */
enum { ERR_DRIVER_UNSPECIFIED };

/*
  driverOpen assumes that the driver has a function called 'getAPIVersion' 
  which returns the API version for which the driver was written 
  */
EXTERN_UTIL Error driverOpen( const char *name, int *apiVersion, 
                              void **driverStruct );
EXTERN_UTIL Error driverClose( void *driverStruct );

EXTERN_UTIL Error driverLoadFunctions( SharedLibrary shlib, int functionCount,
                                       sDriverFunctionInfo *driverFunctions, 
                                       void *driverStruct );
