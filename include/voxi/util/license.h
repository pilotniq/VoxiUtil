/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Software License Handling
  
  Using cryptographically signed license files
*/

#include <time.h>
#include <openssl/x509.h>
#include <openssl/evp.h>  /* High level cryptography functions */

#include <voxi/util/err.h>

/* Local EXTERN macro
   Note: Since the EXTERN macro may be redefined in other .h files, the
   following macro sequence must occur after any other inclusion
   made in this .h file. */
#ifdef EXTERN
#  undef EXTERN
#endif
#ifdef LIB_UTIL_INTERNAL
#  define EXTERN extern
#else
#  ifdef WIN32
#    define EXTERN __declspec(dllimport)
#  else
#    define EXTERN extern
#  endif /* WIN32 */
#endif /* LIB_UTIL_INTERNAL */

typedef EVP_PKEY *PublicKey;
typedef struct sSoftwareLicense *SoftwareLicense;
typedef X509 *VoxiCertificate;

/*
  Global variables
*/

EXTERN VoxiCertificate voxiRootCACertificate;

/** 
 * Initialize the license module.
 */
void license_init();

/**
 * Read certificate from file.
 */
Error license_readCertificate( const char *fileName, VoxiCertificate *certificate );

/**
 * Get the public key from a certificate.
 */
Error license_certificate_getPublicKey( VoxiCertificate certificate, 
                                        PublicKey *publicKey );
/**
 * Read a license from file.
 */
Error license_read( const char *fileName, PublicKey publicKey, 
                    SoftwareLicense *license );
/**
 * Check that a license is valid.
 */
Error license_isValid( SoftwareLicense license, const char *product, 
                       char **invalidReason );

/**
 * Returns the serial number of a license.
 */
int license_getSerialNumber( SoftwareLicense license );

/**
 * Returns the licensee name of a license.
 */
const char *license_getLicensee( SoftwareLicense license );

/**
 * Return the expiry date of a license.
 */
time_t license_getExpiryDate( SoftwareLicense license );

/**
 * Read a license from file using the public key from the given
 * certificate, validate it, and check its expiry date.
 *
 * Will return NULL if the license is valid for the given product name
 * and has not expired.
 */
Error license_checkProductLicense(VoxiCertificate certificate,
                                  char *productName,
                                  char *licenseFilename,
                                  Boolean printMessage);



