/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Shared library functions for Voxi
*/

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#ifdef WIN32
#include <windows.h>

#else
#include <dlfcn.h> /* Probably glibc/Linux specific? */
#endif

#define DEBUG_MESSAGES 0
#include <voxi/alwaysInclude.h>
#include <voxi/util/err.h>

#include <voxi/util/shlib.h>

CVSID("$Id$");

#ifndef LINK_STATIC

/**
   Make sure the shared library filename is "correct" (or at least
   _supposedly_ correct) according to the principles of the actual
   operating system being used. Takes an unquoted string as input.
   Returns a "correct" filename, which should be freed by the caller.
*/
static char *shlib_mangle_filename(const char *filename)
{
  char *realFilename;
  
#ifdef WIN32
  /* Add ".dll" if needed, and remove any "lib" prefix or ".so" suffix. */
  char *fname = (char *)filename;
  char *tmpPtr;
  
  realFilename = (char *)malloc(strlen(filename) + strlen(".dll") + 1);
  
  assert (realFilename != NULL);
  /* Remove "lib" prefix in Unix-style names such as "libfoo". Also,
     take into account that "bar/lib/libfoo" should be changed into
     "bar/lib/foo" (not "bar/libfoo"). */
  if ((tmpPtr = strstr(filename, "lib")) != NULL) {
    char *forwardslashPos = strrchr(filename, '/');
    char *backslashPos = strrchr(filename, '\\');
    char *slashPos;
    slashPos = (forwardslashPos > backslashPos) ? forwardslashPos : backslashPos;
    if ((tmpPtr == filename) && (slashPos == NULL)) {
      /* String starts with "lib", and no directory paths seem to be present.
         Just remove the starting "lib".
         "libfoo" -> "foo" */
      fname = (char *)(filename + 3);
      strcpy(realFilename, fname);
    }
    else if (slashPos != NULL) {
      /* Does not start with "lib", but there seems to be a path.
         If "lib" is in the beginning of the last element, remove it.
         "bar/lib/libfoo" -> "bar/lib/foo" */
      strcpy(realFilename, fname);
      if (strstr(slashPos, "lib") == slashPos+1) {
        strcpy(realFilename + (slashPos-fname) + 1, slashPos + 4);
      }
    }
    else {
      /* "lib" not in beginning, and no path elements present.
         (name of type "foolib"). Just copy. */
      strcpy(realFilename, fname);
    }
  }
  else
    strcpy(realFilename, fname);
    
  if ((tmpPtr = strstr(realFilename, ".so")) != NULL)
    if (strlen(tmpPtr) == 3)    /* Only .so at the end should be removed */
      *tmpPtr = '\0';
  if (strstr(realFilename, ".dll") == NULL)
    strcat(realFilename, ".dll");
  
#else  /* Linux, etc */
  realFilename = strdup(filename);
#endif

  DEBUG("shlib_mangle_filename: result is '%s'\n", realFilename);
  
  return realFilename;
}

Error shlib_open( const char *filename, SharedLibrary *shlib )
{
  Error error = NULL;
  char *realFilename = shlib_mangle_filename(filename);

  DEBUG("enter\n");
  
  assert( shlib != NULL );
  DEBUG("shlib_open(\"%s\", ...)\n", filename);

#ifndef WIN32                   /* Linux */
  /*
    RTLD_GLOBAL is neccessary for ALSA, in order for shared libraries to 
    properly load other shared libraryes...
  */
  *shlib = dlopen( realFilename, RTLD_LAZY | RTLD_GLOBAL );
  if( *shlib == NULL )
    error = ErrNew( ERR_SHLIB, 0, NULL,"dlerror '%s' opening file '%s'", 
                    dlerror(), realFilename );
#else  /* WIN32 */
  {
    HINSTANCE dllhandle;
    if (!(dllhandle = LoadLibrary(realFilename))) {
      error = ErrNew( ERR_SHLIB, 0, ErrWin32(), 
                      "Win32 LoadLibrary (\"%s\") failed.", realFilename );
      *shlib = NULL;
    }
    else
      *shlib = (SharedLibrary)dllhandle;
  }
#endif /* WIN32 */
  
  DEBUG("shlib_open() --> %p, realFilename=\"%s\"\n", *shlib, realFilename);

  free(realFilename);

  DEBUG("leave\n");

  return error;
}

Error shlib_close( SharedLibrary shlib )
{
#ifndef WIN32                   /* Linux */
  int err;
#else  /* WIN32 */
  BOOL err;
#endif
  
  DEBUG("shlib_close(%p)\n", shlib);
  
#ifndef WIN32                   /* Linux */
  err = dlclose( shlib );
  assert( err == 0 );
  
  return NULL;
  
#else  /* WIN32 */
  err = FreeLibrary((HINSTANCE)shlib);
  if (err == 0)
  {
    Error error;

    error = ErrNew( ERR_SHLIB, 0, ErrWin32(), "Win32 FreeLibrary failed." );

    return error;
  }

  return NULL;
#endif /* WIN32 */
}
#include <stdio.h>

Error shlib_findFunc( void *shlib, const char *name, void **funcPtr )
{
  Error error = NULL;

  DEBUG("shlib_findFunc(%p, \"%s\", ...)\n", shlib, name);

#ifndef WIN32                   /* Linux */
  {
    const char *dlerr;
    
    *funcPtr = dlsym( shlib, name );
    
    dlerr = dlerror();
    if( dlerr != NULL )
      error = ErrNew( ERR_SHLIB, 0, NULL, "shlib error '%s' finding the "
                      "function '%s'.", dlerr, name );
  }

#else  /* WIN32 */
  {
    FARPROC func = GetProcAddress((HINSTANCE)shlib, name);
    if (func == NULL) {
      DEBUG("shlib_findFunc: Error looking up %s\n", name);
      error = ErrNew( ERR_SHLIB, 0, ErrWin32(), 
                      "Win32 GetProcAddress(\"%s\") failed.", name );
    }
    else
      DEBUG("shlib_findFunc: Found %s\n", name);
    *funcPtr = (void *)func;
  }
#endif /* WIN32 */

  return error;
}

#else /* LINK_STATIC */

/* This is a hack to be able to profile also the libraries that 
   are usually dynamically loaded */

#include <voxi/util/hash.h>

typedef struct ShlibHashTable_elem {
  char*name;
  void *funcPtr;
} sShlibHashTable_elem, *ShlibHashTable_elem;

static int shlibHashTable_hash( ShlibHashTable_elem e )
{
  return HashString( e->name );
}

static int shlibHashTable_comp( ShlibHashTable_elem e1, ShlibHashTable_elem e2) 
{
  return strcmp(e1->name, e2->name);
}

static void shlibHashTable_dest( ShlibHashTable_elem e ) 
{
  free(e->name);
  e->name = NULL;  
  free(e);
}

void shlibHashTable_add(HashTable ht, char* name, void *funcPtr)
{
  ShlibHashTable_elem e;
  int addOk;

  e=malloc( sizeof( sShlibHashTable_elem ) );
  assert( e!=NULL );

  e->name=strdup(name);
  e->funcPtr = funcPtr;

  addOk = HashAdd(ht,e);
  assert( addOk );
}


static void shlibHashTable_find(HashTable ht, const char* name, void **funcPtr)
{
  ShlibHashTable_elem res;
  sShlibHashTable_elem e;

  e.name = (char*)name;
  res = HashFind( ht, &e );
  if( res == NULL ) {
    DEBUG("No function called '%s' in the sharedlib..\n",name );
    *funcPtr = NULL;
  }  else {
    *funcPtr = res->funcPtr;
  }
}

/* casting all funcitons to void X() creates compile time warnings... */
#define LOAD(Y) do{ void Y(); shlibHashTable_add( ht, #Y, &Y ); } while(0)


static void  loadCoowa(HashTable ht) 
{
  LOAD( coowa_init);
  LOAD( coowa_shutdownServer);
  LOAD( coowa_connectServer);
  LOAD( coowa_createConnection);
  LOAD( coowa_getConnectionRepositories);
  LOAD( coowa_create);
  LOAD( coowa_getLocalRootObjects);
  LOAD( coowa_getAllWithParent);
  LOAD( coowa_getRecursiveChildrenOfObject);
  LOAD( coowa_save);
  LOAD( coowa_getAllObjects);
  LOAD( coowa_destroy);
  LOAD( coowa_hasActionHandler);
  LOAD( coowa_getActionHandlers);
  LOAD( coowa_callActionHandler);
  LOAD( coowa_getActionPreconditions);
  LOAD( coowa_getActionPostconditions);
  LOAD( coowa_getByPostcondition);
  LOAD( coowa_attrSet);
  LOAD( coowa_attrGet);
  LOAD( coowa_attrGetRaw);
  LOAD( coowa_attrSetShallow);
  LOAD( coowa_attrTest);
  LOAD( coowa_getIsNoMore);
  LOAD( coowa_isInstanceOf);
  LOAD( coowa_getNonInheritedActionHandlers);
  LOAD( coowa_setActionPreconditions);
  LOAD( coowa_setActionPostconditions);
  LOAD( coowa_destroyAttributeValue);
  LOAD( coowa_getAttributes);
  LOAD( coowa_setActionHandlerFunctionName);
  LOAD( coowa_getActionHandlerFunctionName);
  LOAD( coowa_destroyActionHandler);
  LOAD( coowa_createActionHandler);
  LOAD( coowa_createRepository);
  LOAD( coowa_checkRepository);
}


static void  loadSoundDriver(HashTable ht) 
{
  LOAD(openDriver);
  LOAD(closeDriver);
  LOAD(openDevice);
  LOAD(closeDevice);
  LOAD(getMixer);
  LOAD(getBufferedStream);
  LOAD(getFork);
  LOAD(setTimerCallbackFunc);
  LOAD(unsetTimerCallbackFunc);
  LOAD(setDebugState);
  LOAD(setRecordGain);
}


static void  loadCoreLib(HashTable ht) 
{
  LOAD(bo_beliefObject_createHypothetical);
  LOAD(bo_belief_object_isInstanceOf);
  LOAD(bo_beliefs_believeObject);
  LOAD(bo_beliefs_create);
  LOAD(bo_beliefs_destroy);
  LOAD(bo_beliefs_getAllBeliefObjects);
  LOAD(bo_beliefs_getBeliefObject);
  LOAD(bo_beliefs_object_addRelation);
  LOAD(bo_beliefs_object_compare);
  LOAD(bo_beliefs_object_destroy);
  LOAD(bo_beliefs_object_getAttrTypedValue);
  LOAD(bo_beliefs_object_getBeliever);
  LOAD(bo_beliefs_object_getChildren);
  LOAD(bo_beliefs_object_getContents);
  LOAD(bo_beliefs_object_getDebugName);
  LOAD(bo_beliefs_object_getGender);
  LOAD(bo_beliefs_object_getInstances);
  LOAD(bo_beliefs_object_getName);
  LOAD(bo_beliefs_object_getObject);
  LOAD(bo_beliefs_object_getParent);
  LOAD(bo_beliefs_object_hasISARelationship);
  LOAD(bo_beliefs_object_hasRelation);
  LOAD(bo_beliefs_object_isHypothetical);
  LOAD(bo_beliefs_object_setAttrToTypedValue);
  LOAD(bo_beliefs_object_setParent);
  LOAD(bo_beliefs_object_transfer);
  LOAD(bo_beliefs_realizeHypotheticalObject);
  /*  LOAD(compareBeliefs); */
  /*  LOAD(compareBoURI); */
  LOAD(coowaObject_beliefObject_createHypothetical);
  LOAD(coowaObject_belief_object_isInstanceOf);
  LOAD(coowaObject_beliefs_believeObject);
  LOAD(coowaObject_beliefs_create);
  LOAD(coowaObject_beliefs_destroy);
  LOAD(coowaObject_beliefs_getAllBeliefObjects);
  LOAD(coowaObject_beliefs_getBeliefObject);
  LOAD(coowaObject_beliefs_object_addRelation);
  LOAD(coowaObject_beliefs_object_compare);
  LOAD(coowaObject_beliefs_object_destroy);
  LOAD(coowaObject_beliefs_object_getAttrTypedValue);
  LOAD(coowaObject_beliefs_object_getBeliever);
  LOAD(coowaObject_beliefs_object_getChildren);
  LOAD(coowaObject_beliefs_object_getDebugName);
  LOAD(coowaObject_beliefs_object_getGender);
  LOAD(coowaObject_beliefs_object_getInstances);
  LOAD(coowaObject_beliefs_object_getName);
  LOAD(coowaObject_beliefs_object_getObject);
  LOAD(coowaObject_beliefs_object_getParent);
  LOAD(coowaObject_beliefs_object_hasISARelationship);
  LOAD(coowaObject_beliefs_object_hasRelation);
  LOAD(coowaObject_beliefs_object_isHypothetical);
  LOAD(coowaObject_beliefs_object_setAttrToTypedValue);
  LOAD(coowaObject_beliefs_object_setParent);
  LOAD(coowaObject_beliefs_object_transfer);
  LOAD(coowaObject_beliefs_realizeHypotheticalObject);
  LOAD(coowa_attr_conjugations_get);
  LOAD(coowa_attr_words_get);
  LOAD(coowa_attr_words_set);

}

#if 0
static void  loadLibSwedish(HashTable ht) 
{

  LOAD(init_language);
  LOAD(generate_AdjectivePhrase_A);
  LOAD(generate_AdjectivePhrase_A_A);
  LOAD(generate_AdjectivePhrase_A_A_A);
  LOAD(generate_AdjectiveTerminal);
  LOAD(generate_CommonNounIrregularTerminal);
  LOAD(generate_CommonNounPhrase_CN);
  LOAD(generate_CommonNounPhrase_CN_SUFFIX);
  LOAD(generate_CommonNounRootTerminal);
  LOAD(generate_DeterminerTerminal);
  LOAD(generate_ImperativeVerbPhrase);
  LOAD(generate_NounPhrase_AP_CN);
  LOAD(generate_NounPhrase_CN);
  LOAD(generate_NounPhrase_DET_AP_CN);
  LOAD(generate_NounPhrase_DET_CN);
  LOAD(generate_NounPhrase_PN);
  LOAD(generate_NounSuffixTerminal);
  LOAD(generate_ProperNounTerminal);
  LOAD(generate_VerbPhrase_V);
  LOAD(generate_VerbPhrase_V_NP);
  LOAD(generate_VerbTerminal);
  LOAD(resolve_AdjectivePhrase);
  LOAD(resolve_CommonNounPhrase_CN);
  LOAD(resolve_CommonNounPhrase_CN_SUFFIX);
  LOAD(resolve_ImperativeVerbPhrase);
  LOAD(resolve_NounPhrase_AP_CN);
  LOAD(resolve_NounPhrase_CN);
  LOAD(resolve_NounPhrase_DET_AP_CN);
  LOAD(resolve_NounPhrase_PN);
  LOAD(resolve_VerbPhrase_V);
  LOAD(resolve_VerbPhrase_V_NP);

}
#endif

static void  loadLibSwedishRepository(HashTable ht) 
{
  LOAD(mi_swedishLanguage_init);
  LOAD(mi_swedishLanguage_getPronounciation);
}

#if 0
static void  loadMp3player(HashTable ht) 
{
  LOAD(mi_continue_song);
  LOAD(mi_decrease_volume);
  LOAD(mi_increase_volume);
  LOAD(mi_mp3player_init);
  LOAD(mi_mp3player_isASongPlaying);
  LOAD(mi_mp3player_playNextTrack);
  LOAD(mi_mp3player_playSongs);
  LOAD(mi_mp3player_stopPlaying);
  LOAD(mi_objectAdded);
  LOAD(mi_pause_song);
  LOAD(mi_playlist_addSongs);
  LOAD(mi_playlist_clear);
  LOAD(mi_playlist_popNextSong);
  LOAD(mi_read_the_playlist);
  LOAD(mi_restore_volume);
  LOAD(mi_what_is_playing);
  LOAD(mp3player_isASongPlaying);
  LOAD(mp3player_playNext);
  LOAD(mp3player_stopPlaying);
  LOAD(playlist_addSongs);
  LOAD(playlist_clear);
  LOAD(playlist_init);
  LOAD(playlist_popNextSong);
}
#endif

Error shlib_open( const char *filename, SharedLibrary *shlib )
{
  HashTable ht;

  assert( shlib != NULL );

  ht = HashCreateTable( 100, 
                         (HashFuncPtr)    shlibHashTable_hash,
                         (CompFuncPtr)    shlibHashTable_comp,
                         (DestroyFuncPtr) shlibHashTable_dest );

  assert( ht != NULL );

  DIAG("Opening shlib '%s'\n",filename );

  if( strcmp( "libcoowa_g.so", filename ) == 0 )
    loadCoowa(ht);
  else if( strcmp( "coreLib.so", filename ) == 0 )
    loadCoreLib(ht);
  else if( strcmp( "../lib/libswedish-repository.so", filename ) == 0 )
    loadLibSwedishRepository(ht);
  else if( strcmp( "alsaSound_05.so", filename ) == 0 )
    loadSoundDriver(ht);
  else if( strcmp( "nullSound.so", filename ) == 0 )
    loadSoundDriver(ht);
  else {
    Error application_shlib_open( const char *filename, HashTable ht );
    Error error;
    error = application_shlib_open( filename,ht );

    if( error != NULL ) {
      fprintf(stderr, "Cant lookup the functions for '%s'. You must hack shlib.c...\n",filename );
      DEBUG("leave-error\n");
      return ErrNew( ERR_SHLIB, 0, error,"Cant \"load\" file '%s'", filename );
    }
  }

  *shlib = (SharedLibrary)ht;
  return NULL;
}

Error shlib_close( SharedLibrary shlib )
{
  HashDestroyTable( (HashTable)shlib );
  return NULL;
}

Error shlib_findFunc( void *shlib, const char *name, void **funcPtr )
{
  shlibHashTable_find((HashTable) shlib, name, funcPtr);

  if( *funcPtr == NULL )
    return ErrNew( ERR_SHLIB, 0, NULL, "shlib error. Can't find function '%s'.", name );
  else 
    return NULL;
}

#endif /* LINK_STATIC */


