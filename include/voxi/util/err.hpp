/*
 * VoxiError.hpp
 *
 * A C++ class encapsulating a Voxi Error, so that it can be thrown as an exception
 *
 */

#include <voxi/util/err.h>

class VoxiError
{
public:
  VoxiError( Error error ) : error( error ) {};
  ~VoxiError() { };

  Error getError() { return error; }

private:
  Error error;
};