/*
 * driver.h
 *
 * Helper code for drivers - that is code in a shared library/DLL loaded at 
 * runtime, containing functions with known names, which are stored as
 * function pointers in a driver struct
 */

#include <assert.h>
#include <voxi/util/driver.h>
#include <voxi/util/err.h>
#include <voxi/util/shlib.h>

/*
 *  Type definitions
 */

typedef Error (*DriverGetAPIVersionFuncPtr)( int *version );

/*
  driverOpen assumes that the driver has a function called 'getAPIVersion' 
  which returns the API version for which the driver was written 
  */
Error driverOpen( const char *name, int *apiVersion, SharedLibrary *shlib )
{
  Error error;
  DriverGetAPIVersionFuncPtr getAPIVersionFunc;

  error = shlib_open( name, shlib );
  if( error != NULL )
  {
    error = ErrNew( ERR_DRIVER, ERR_DRIVER_UNSPECIFIED, error, 
                    "Failed to open driver shared library/DLL '%s'", name );
    goto OPEN_DRIVER_FAIL_1;
  }

  error = shlib_findFunc( *shlib, "getAPIVersion", (void **) &getAPIVersionFunc);
  if( error != NULL )
  {
    error = ErrNew( ERR_DRIVER, ERR_DRIVER_UNSPECIFIED, error, 
                    "Driver did not have 'getAPIVersion' function" );
    goto OPEN_DRIVER_FAIL_2;
  }

  /* Check the version number */
  assert( getAPIVersionFunc != NULL );

  error = getAPIVersionFunc( apiVersion );
  if( error != NULL )
  {
    error = ErrNew( ERR_DRIVER, ERR_DRIVER_UNSPECIFIED, NULL, 
                    "Failed to get driver API Version." );
    goto OPEN_DRIVER_FAIL_2;
  }

  /* Success! */

  assert( error == NULL );

  return NULL;

OPEN_DRIVER_FAIL_2:
  shlib_close( *shlib );

OPEN_DRIVER_FAIL_1:
  assert( error != NULL );

  return error;
}

Error driverLoadFunctions( SharedLibrary shlib, int functionCount, 
                           sDriverFunctionInfo *driverFunctions, void *driverStruct )
{
  int i;
  Error error;

  for( i = 0; i < functionCount; i++ )
  {
    error = shlib_findFunc( shlib, driverFunctions[i].name, 
                            (void **) driverStruct+driverFunctions[i].offset);
    if( error != NULL )
    {
      error = ErrNew( ERR_DRIVER, ERR_DRIVER_UNSPECIFIED, error, 
                      "Driver file does not have function '%s' ", 
                      driverFunctions[i].name );
      goto DRIVER_LOAD_FAIL;
    }
  }

  return NULL;

DRIVER_LOAD_FAIL:
  assert( error != NULL );

  return error;
}

