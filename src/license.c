/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Software License Handling
  
  Using cryptographically signed license files
*/

#include <assert.h>
#include <stdio.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

#ifdef WIN32
#include <voxi/util/win32_glue.h> /* strcasecmp */
#endif /* WIN32 */
#include <voxi/util/err.h>
#include <voxi/util/file.h>
#include <voxi/util/path.h>
#include <voxi/alwaysInclude.h>

#include <voxi/util/license.h>

CVSID("$Id$");

#define LICENSE_FILE_MAX_LINE_LENGTH 1024

typedef struct sSoftwareLicense
{
  int serialNumber;
  char *licensee;
  time_t expiryDate;
  char *product;
} sSoftwareLicense;

/* This is Voxi's root CA, exported from the command:
   
   openssl x509 -in rootCA/cacert.pem -C -out rootCAcert.c
   
*/
static unsigned char voxiRootCACertificateData[700]={
0x30,0x82,0x02,0xB8,0x30,0x82,0x02,0x21,0xA0,0x03,0x02,0x01,0x02,0x02,0x01,0x00,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x04,0x05,0x00,0x30,0x4D,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x53,0x45,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x07,0x13,0x09,0x53,0x74,0x6F,0x63,0x6B,0x68,0x6F,0x6C,0x6D,0x31,0x10,0x30,0x0E,0x06,0x03,0x55,0x04,0x0A,0x13,0x07,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x31,0x18,0x30,0x16,0x06,0x03,0x55,0x04,0x03,0x13,0x0F,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x20,0x52,0x6F,0x6F,0x74,0x20,0x43,0x41,0x30,0x1E,0x17,0x0D,0x30,0x32,0x30,0x32,0x30,0x35,0x31,0x35,0x33,0x33,0x32,0x34,0x5A,0x17,0x0D,0x31,0x32,0x30,0x32,0x30,0x33,0x31,0x35,0x33,0x33,0x32,0x34,0x5A,0x30,0x4D,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x53,0x45,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x07,0x13,0x09,0x53,0x74,0x6F,0x63,0x6B,0x68,0x6F,0x6C,0x6D,0x31,0x10,0x30,0x0E,0x06,0x03,0x55,0x04,0x0A,0x13,0x07,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x31,0x18,0x30,0x16,0x06,0x03,0x55,0x04,0x03,0x13,0x0F,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x20,0x52,0x6F,0x6F,0x74,0x20,0x43,0x41,0x30,0x81,0x9F,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x01,0x05,0x00,0x03,0x81,0x8D,0x00,0x30,0x81,0x89,0x02,0x81,0x81,0x00,0xD8,0x9F,0x64,0x7E,0xBD,0x6E,0xD1,0x1C,0xB6,0x08,0x0C,0xB3,0x34,0xCC,0x67,0xEC,0x26,0x94,0xDE,0xD1,0xC9,0xF5,0xB9,0x1E,0x78,0xF2,0xA3,0x88,0x6F,0x3C,0x49,0x9F,0x5D,0x77,0x2E,0x42,0x26,0x12,0xFA,0xC2,0xA6,0xD0,0x09,0x97,0x01,0xBC,0x9C,0x59,0x20,0x6C,0x15,0xB5,0x9A,0x85,0x06,0xDF,0x84,0x90,0x0E,0xE5,0x3B,0x05,0x8F,0x40,0x07,0x86,0x96,0x33,0x0E,0xD7,0xA7,0x3E,0xB0,0xD1,0x4D,0xFF,0x64,0x6D,0x8A,0x69,0x75,0xEC,0x2A,0x0E,0x93,0x4E,0xE7,0x47,0xED,0x93,0x83,0x27,0x2B,0x9A,0xB3,0x02,0xF9,0x76,0xB9,0x18,0xC3,0x3B,0x28,0x4A,0x06,0xC3,0x26,0xB8,0xFA,0xFA,0xBC,0x97,0x01,0x7C,0xA9,0xB9,0xFA,0x1F,0x05,0xAA,0x5A,0x38,0x46,0x14,0x3A,0xC5,0xDB,0xD5,0x02,0x03,0x01,0x00,0x01,0xA3,0x81,0xA7,0x30,0x81,0xA4,0x30,0x1D,0x06,0x03,0x55,0x1D,0x0E,0x04,0x16,0x04,0x14,0xC1,0x07,0xC2,0xD7,0x95,0x88,0x5D,0x5D,0x2C,0xE1,0xDA,0x80,0xAF,0x75,0xCD,0xAE,0x6E,0x1F,0x87,0xBB,0x30,0x75,0x06,0x03,0x55,0x1D,0x23,0x04,0x6E,0x30,0x6C,0x80,0x14,0xC1,0x07,0xC2,0xD7,0x95,0x88,0x5D,0x5D,0x2C,0xE1,0xDA,0x80,0xAF,0x75,0xCD,0xAE,0x6E,0x1F,0x87,0xBB,0xA1,0x51,0xA4,0x4F,0x30,0x4D,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x53,0x45,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x07,0x13,0x09,0x53,0x74,0x6F,0x63,0x6B,0x68,0x6F,0x6C,0x6D,0x31,0x10,0x30,0x0E,0x06,0x03,0x55,0x04,0x0A,0x13,0x07,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x31,0x18,0x30,0x16,0x06,0x03,0x55,0x04,0x03,0x13,0x0F,0x56,0x6F,0x78,0x69,0x20,0x41,0x42,0x20,0x52,0x6F,0x6F,0x74,0x20,0x43,0x41,0x82,0x01,0x00,0x30,0x0C,0x06,0x03,0x55,0x1D,0x13,0x04,0x05,0x30,0x03,0x01,0x01,0xFF,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x04,0x05,0x00,0x03,0x81,0x81,0x00,0xB5,0x87,0x37,0x76,0x60,0x95,0x8A,0x36,0x05,0x43,0xEA,0x46,0x3A,0x14,0x19,0x57,0x29,0xD7,0xF1,0x99,0xD0,0xD8,0x4B,0x8C,0x1C,0xAF,0x28,0x23,0x39,0x52,0x99,0x96,0x32,0x6B,0x6A,0xD0,0xB7,0xF8,0xF1,0x15,0xE4,0xB0,0xE3,0x7C,0x63,0xCC,0x27,0x0C,0xA8,0xE5,0x2D,0xA9,0x5C,0x44,0xDF,0xCB,0xDA,0xC1,0x50,0x6F,0xD8,0x8D,0x1B,0x73,0x32,0x55,0x8D,0x09,0x5D,0x31,0x41,0x5E,0xDC,0x6A,0x0F,0xD5,0xE7,0xB1,0x38,0x24,0x0F,0x1D,0xC8,0x01,0xC4,0x70,0xB1,0x9A,0xA2,0x6B,0xDD,0xD9,0xD2,0x00,0xA8,0x9F,0x8C,0xF3,0x48,0xF9,0xBF,0x51,0xE3,0xBE,0x9C,0x7C,0x24,0x42,0xF0,0x98,0xB2,0x9C,0x82,0x09,0xE8,0x50,0x43,0xF4,0x3B,0x85,0x8A,0x39,0x68,0x9E,0xE6,0x70,0xA1,0x68,
};

/* exported variable */
VoxiCertificate voxiRootCACertificate;
   
/*
 * Start of Code
 */

void license_init()
{
  unsigned char *tempPtr = voxiRootCACertificateData;
  
  /* This is neccesary, otherwise the verification of the VLCA certificate
     fails, with an "Unknown Message Digest Algorithm error" 
  */
  OpenSSL_add_all_algorithms();
  
  voxiRootCACertificate = d2i_X509( NULL, &tempPtr, 
                                    sizeof( voxiRootCACertificateData ) );
  assert( voxiRootCACertificate != NULL );
}

Error license_certificate_getPublicKey( VoxiCertificate certificate, 
                                        PublicKey *publicKey )
{
  X509_PUBKEY *tempPubKey;
  
  tempPubKey = X509_get_X509_PUBKEY( certificate );
  
  assert( tempPubKey != NULL );
  
  *publicKey = X509_PUBKEY_get( tempPubKey );
  
  assert( *publicKey != NULL );
  
  return NULL;
}


Error license_readCertificate( const char *fileName, X509 **certificate )
{
  FILE *file;
  
  file = fopen( fileName, "rb" );
  assert( file != NULL );
  
  PEM_read_X509( file, certificate, NULL, NULL );
  
  /* d2i_X509_fp( file, certificate ); */
  
  fclose( file );
  
  return NULL;
}

/* A license file consists of:
   
   3                      (license file format version)
   <Serial number>        
   <Licensee>
   <ExpiryDate>           (YYYYMMDD)
   <Product>
   <PEM-format X.509 certificate for Voxi License CA which signed the license>
   <128 byte SHA1 signature of the file>  (in hex format)
   
   Checks: 
   
   * The Voxi License CA certificate signature will be checked against the
     well-known Voxi Public Key
   
   * The Voci License CA certificate contains a flag that indicates that it is
     certified to license software licenses.
     
   * If we ever revoke VLCA certificates, then the VLCA certificate should be
     checked against a Certificate Revocation List (CRL)
   
   * It is checked that the MD5-signature by the License CA is correct

   File is searched for in the following places, and order:
   * fileName unmodified
   * VOXI_ISI_DIR/licenses/fileName
   * ../licenses/fileName
   
*/
Error license_read( const char *fileName, PublicKey publicKey, 
                    SoftwareLicense *license )
{
  /* char licenseFilename[ FILENAME_MAX ]; */
  FILE *licenseFile;
  Error error;
  char buf[ LICENSE_FILE_MAX_LINE_LENGTH + 1 ];
  int err, tempInt;
  struct tm tm;
  char buf2[ 5 ];
  X509 *VLCAcertificate;
  PublicKey VLCAPublicKey;
  unsigned char *signature; /* [ SHA_DIGEST_LENGTH ]; -- EVP_PKEY_size( pkey ) */
  char *signedPart = NULL;
  long filePtr;
  EVP_MD_CTX signatureContext;
  int maxSignatureLength;
  int signatureLength;
  int valid;

  char *fileNames[3];
  char *license_fileName, *isiDir;
  int i;
  
  /*
   * Look for the license file
   */
  
  isiDir = getenv("VOXI_ISI_DIR");
  isiDir = isiDir ? isiDir : "..";
  
  license_fileName = path_concat_dir("licenses", fileName);
  fileNames[0] = strdup(fileName);
  fileNames[1] = path_concat_dir(isiDir, license_fileName);
  fileNames[2] = path_concat_dir("..", license_fileName);
  
  for (i = 0, licenseFile = NULL; (i < 3) && (licenseFile == NULL); i++)
    licenseFile = fopen( fileNames[i], "rb" );
  
  for (i = 0; i < 3; i++) free(fileNames[i]);
  free(license_fileName);

  if( licenseFile == NULL ) {
    error = ErrNew( ERR_UNKNOWN, 0, ErrErrno(), 
                    "Failed to open the license file '%s'.", fileName );
    
    goto LICENSE_READ_FAIL_1;
  }
  
  /*
   *  Create the license struct
   */
  assert( license != NULL );
  
  (*license) = malloc( sizeof( sSoftwareLicense ) );
  assert( (*license) != NULL );
  
  /* Read & parse the license info */
  err = file_readLine( licenseFile, buf, LICENSE_FILE_MAX_LINE_LENGTH + 1 );
  assert( err == 0 );
  
  /* get the file format version number */
  tempInt = atoi( buf );
  if( tempInt != 3 )
  {
    error = ErrNew( ERR_UNKNOWN, 0, NULL, "License file file format version is "
                    "%s, only version 3 supported.", buf );
    goto LICENSE_READ_FAIL_2;
  }
  
  /* Read the license serial number */
  err = file_readLine( licenseFile, buf, LICENSE_FILE_MAX_LINE_LENGTH + 1 );
  assert( err == 0 );
  
  /* get the file format version number */
  tempInt = atoi( buf );
  
  (*license)->serialNumber = atoi( buf );
  
  /* Read the licensee */
  err = file_readLine( licenseFile, buf, LICENSE_FILE_MAX_LINE_LENGTH + 1 );
  assert( err == 0 );
  
  (*license)->licensee = strdup( buf );
  assert( (*license)->licensee != NULL );
  
  /* Read the expiry date */
  err = file_readLine( licenseFile, buf, LICENSE_FILE_MAX_LINE_LENGTH + 1 );
  assert( err == 0 );
  
  memset( &tm, 0, sizeof( tm ) );
  
  /* tm.tm_sec = tm.tm_min = tm.tm_hour = tm.tm_wday = tm.tm_isdst = 0; */
  
  assert( strlen( buf ) == 8 );
  
  strncpy( buf2, buf, 4 );
  tm.tm_year = atoi( buf2 ) - 1900;
  
  strncpy( buf2, buf+4, 2 );
  buf2[ 2 ] = '\0';
  tm.tm_mon = atoi( buf2 ) - 1;
  
  strncpy( buf2, buf+6, 2 );
  tm.tm_mday = atoi( buf2 );
  
  (*license)->expiryDate = mktime( &tm );
  
  /* Read the expiry date */
  err = file_readLine( licenseFile, buf, LICENSE_FILE_MAX_LINE_LENGTH + 1 );
  assert( err == 0 );
  
  (*license)->product = strdup( buf );
  
  /* Read the Voxi License Certificate Authority Certificate */
  
  VLCAcertificate = NULL;
  
  PEM_read_X509( licenseFile, &VLCAcertificate, NULL, NULL );
  
  error = license_certificate_getPublicKey( VLCAcertificate, &VLCAPublicKey );
  assert( error == NULL );
  
  /* TODO: Check that the VLCA license is actually certified to sign software 
     licenses
  */
  
  /* The file pointer should now be at the end of the data which should be 
     signed */
  filePtr = ftell( licenseFile );
  
  /* Read the signature */
  
  maxSignatureLength = EVP_PKEY_size( VLCAPublicKey );

  signature = malloc( maxSignatureLength );
  
  error = file_readHexAsBinary( licenseFile, signature, maxSignatureLength );
  if( error != NULL )
  {
    error = ErrNew( ERR_UNKNOWN, 0, error, "Failed to read the license "
                    "signature" );
    goto LICENSE_READ_FAIL_3;
  }
  signatureLength = maxSignatureLength;
  
#if 0  
  signatureLength = fread( signature, 1, maxSignatureLength, licenseFile );
#endif
  /* Verify the file signature */
  /* re-read the data to be signed */
  rewind( licenseFile );
  
  signedPart = malloc( filePtr );
  
  /* re-open as binary? */
#if 0
  for( bufPtr = 0; bufPtr < filePtr; )
  {
#endif
    /* tempInt = fread( signedPart + bufPtr, filePtr - bufPtr, 1, licenseFile );*/
    tempInt = fread( signedPart, filePtr, 1, licenseFile );
    assert( tempInt == 1 );
#if 0
    bufPtr += tempInt;
  }

  assert( tempInt == filePtr );
#endif
#if 0 /* Apparently this function doesn't exist until openssl version >0.9.6c */
  err = EVP_MD_CTX_init( &signatureContext );
  assert( err == 0 );
#endif
  EVP_VerifyInit( &signatureContext, EVP_sha1() );
  
  EVP_VerifyUpdate( &signatureContext, signedPart, filePtr );
  
  valid = EVP_VerifyFinal( &signatureContext, signature, signatureLength, 
                           VLCAPublicKey );
  if( !valid )
  {
    error = ErrNew( ERR_UNKNOWN, 0, NULL, "Invalid License" );
    goto LICENSE_READ_FAIL_4;
  }
  
  /* Must also verify the VLCA Certificate to make sure that it is signed by 
     the Voxi Root CA */
  valid = X509_verify( VLCAcertificate, publicKey );
  if( !valid )
  {
    error = ErrNew( ERR_UNKNOWN, 0, NULL, "Invalid VLCA License" );
    goto LICENSE_READ_FAIL_4;
  }
  
  /* EVP_MD_CTX_cleanup( &signatureContext ); */
  free( signature );
  free( signedPart );
  

  assert( error == NULL );
  
  return NULL;
  
 LICENSE_READ_FAIL_4:
  /* EVP_MD_CTX_cleanup( &signatureContext ); */
  
 LICENSE_READ_FAIL_3:
  free( signedPart );
  free( signature );
  
 LICENSE_READ_FAIL_2:
  fclose( licenseFile );
  free( (*license) );
  *license = NULL;
  
 LICENSE_READ_FAIL_1:
  assert( error != NULL );
    
  return error;
}

Error license_isValid( SoftwareLicense license, const char *product, 
                       char **invalidReason )
{
  char buf[ 1024 ];
  time_t now = time( NULL );
  
  assert( invalidReason != NULL );
  
  /* TODO: Check that the license is for the right product */
  
  assert( license != NULL );
  if( license->expiryDate < now )
  {
    char *tempStr1, *tempStr2;
    
    tempStr1 = strdup( ctime( &license->expiryDate ) );
    tempStr2 = strdup( ctime( &now ) );
    
    snprintf( buf, sizeof(buf), "License expired on %s, it is now %s.\n", 
              tempStr1, tempStr2 );
    free( tempStr1 );
    free( tempStr2 );
    
    *invalidReason = strdup( buf );
  }
  else if( strcasecmp( license->product, product ) != 0 )
  {
    snprintf( buf, sizeof(buf), "License mismatch." );
    
    *invalidReason = strdup( buf );
  }
  else
    *invalidReason = NULL;
  
  return NULL;
}

const char *license_getLicensee( SoftwareLicense license )
{
  return license->licensee;
}

int license_getSerialNumber( SoftwareLicense license )
{
  return license->serialNumber;
}

time_t license_getExpiryDate( SoftwareLicense license )
{
  return license->expiryDate;
}

Error license_checkProductLicense(VoxiCertificate certificate,
                                  char *productName,
                                  char *licenseFilename,
                                  Boolean printMessage)
{
  Error error = NULL;
  SoftwareLicense license;
  PublicKey voxiRootCAPublicKey;
  char *tempStr;
  
  error = license_certificate_getPublicKey( certificate,
                                            &voxiRootCAPublicKey );
  if (error != NULL)
    goto ERR_RETURN;

  error = license_read( licenseFilename, voxiRootCAPublicKey, &license );

  if( error != NULL )
    goto ERR_RETURN;

  error = license_isValid( license, productName, &tempStr );
  if( error != NULL )
    goto ERR_RETURN;

  if( tempStr == NULL ) {
    time_t expiryDate;
      
    expiryDate = license_getExpiryDate( license );

    if (printMessage) {
      printf("%s license #%d, licensed to %s.\nExpires on %s.\n",
             productName,
             license_getSerialNumber( license ),
             license_getLicensee( license ), 
             ctime( &expiryDate ) );
    }
  }
  else {
    error = ErrNew( ERR_APP, 0, ErrNew( ERR_APP, 0, NULL, tempStr ), 
                    "License invalid." );
    goto ERR_RETURN;
  }
  
 ERR_RETURN:
  return error;
}
