/*
 * stateMachine.c
 *
 * State machine, initially written to have the same functionality as the Nuance 
 * DialogBuilder.
 *
 */

#include <assert.h>
#include <stdlib.h>

#include <voxi/util/bag.h>
#include <voxi/util/err.h>
#include <voxi/util/mem.h>
#include <voxi/util/stateMachine.h>
#include <voxi/util/logging.h>

#define LOG_LEVEL   LOGLEVEL_ERROR

typedef struct sStateMachineDefinition
{
  int machineCount;
  Bag classes;
  Bag states;
} sStateMachineDefinition;

typedef struct sStateMachine 
{
  StateMachineDefinition definition;
  StateMachineState currentState;
  void *userData;
  StateMachineState nextState;
  Boolean immediateExit;

  /* Nuance Semantics means that you can't set the next state more than once */
  Boolean nuanceSemantics;
} sStateMachine;

typedef struct sStateClass 
{
  char *name;
  StateMachineDefinition machineDefinition;
  StateFunction entryFunc;
  StateFunction exitFunc;
  int stateCount;
  void *userData;
} sStateClass;

typedef struct sStateMachineState
{
  char *name;
  StateMachineDefinition machineDefinition;
  StateClass cls;
  StateFunction entryFunc, exitFunc;
} sStateMachineState;

Error stateMachine_defCreate( StateMachineDefinition *def )
{
  Error error;

  error = emalloc( (void **) def, sizeof( sStateMachineDefinition ) );
  if( error != NULL )
    return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_OUT_OF_MEMORY, error, 
      "Failed to allocate %d bytes of memory for state machine definition",
      sizeof( sStateMachineDefinition ) );

  (*def)->machineCount = 0;
  (*def)->classes = bagCreate( 32, 32, NULL );
  (*def)->states = bagCreate( 32, 32, NULL );

  return NULL;
}

Error stateMachine_defDestroy( StateMachineDefinition def )
{
  Error error;

  if( def->machineCount != 0 )
  {
    return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_DEF_HAS_MACHINES, NULL,
      "The state machine definition has %d state machines refering to it.",
      def->machineCount );
  }

  /* Destroy all states */
  while( bagNoElements( def->states) > 0 )
  {
    error = stateMachine_destroyState( 
                  (StateMachineState) bagGetRandomElement( def->states ) );
    if( error != NULL )
      return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, error, 
                     "Could not destroy state." );
  }

  /* Destroy all state classes */
  while( bagNoElements( def->classes) > 0 )
  {
    error = stateMachine_destroyClass( (StateClass) bagGetRandomElement( def->classes ) );
    if( error != NULL )
      return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, error, "stateMachine_destroyClass failed." );
  }

  bagDestroy( def->states, NULL );
  bagDestroy( def->classes, NULL );
  
  free( def );
  
  return NULL;
}

Error stateMachine_create( StateMachineDefinition def, StateMachine *machine, 
                           void *userData, Boolean nuanceSemantics )
{
  Error error;

  error = emalloc( (void **) machine, sizeof( sStateMachine ) );
  if( error != NULL )
    return error;

  (*machine)->definition = def;
  (*machine)->currentState = NULL;
  (*machine)->userData = userData;
  (*machine)->nextState = NULL;
  (*machine)->immediateExit = FALSE;
  (*machine)->nuanceSemantics = nuanceSemantics;

  def->machineCount++;

  return NULL;
}

Error stateMachine_destroy( StateMachine machine )
{
  StateMachineDefinition def;

  def = machine->definition;

  free( machine );
  
  def->machineCount--;

  return NULL;
}

Error stateMachine_createClass( StateMachineDefinition def, StateClass *result, 
                                const char *name, 
                                StateFunction entryFunc, StateFunction exitFunc,
                                void *userData )
{
  Error error;

  error = emalloc( (void **) result, sizeof( sStateClass ) );
  if( error != NULL )
    return error;

  (*result)->name = strdup( name );
  (*result)->machineDefinition = def;
  (*result)->entryFunc = entryFunc;
  (*result)->exitFunc = exitFunc;
  (*result)->stateCount = 0;
  (*result)->userData = userData;

  bagAdd( def->classes, *result );

  return NULL;
}

Error stateMachine_destroyClass( StateClass stateClass )
{
  if( stateClass->stateCount != 0 )
    return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, NULL,
      "The state class still had %d states.", stateClass->stateCount );

  bagRemove( stateClass->machineDefinition->classes, stateClass );

  free( stateClass->name ); 

  return NULL;
}

Error stateMachine_createState( StateMachineDefinition def, StateMachineState *state, 
                                const char *name, 
                                StateClass stateClass, StateFunction entryFunc, 
                                StateFunction exitFunc )
{
  Error error;

  error = emalloc( (void **) state, sizeof( sStateMachineState ) );
  if( error != NULL )
    return error;

  (*state)->name = strdup( name );
  (*state)->machineDefinition = def;
  (*state)->cls = stateClass;
  (*state)->entryFunc = entryFunc;
  (*state)->exitFunc = exitFunc;

  bagAdd( def->states, *state );

  if( stateClass != NULL )
    stateClass->stateCount++;

  return NULL;
}

Error stateMachine_destroyState( StateMachineState state )
{
  if( state->cls != NULL )
    state->cls->stateCount--;

  bagRemove( state->machineDefinition->states, state );
  free( state->name );

  free( state );

  return NULL;
}

void *stateMachine_classGetUserData( StateClass cls )
{
  assert( cls != NULL );

  return cls->userData;
}

StateClass stateMachine_stateGetClass( StateMachineState state )
{
  assert( state != NULL );

  return state->cls;
}


StateMachineDefinition stateMachine_stateGetMachineDefinition( StateMachineState state )
{
  return state->machineDefinition;
}

void *stateMachine_getUserData( StateMachine machine )
{
  return machine->userData;
}

Error stateMachine_setNextState( StateMachine machine, 
                                 StateMachineState nextState )
{
  /* should check that the next state actually is a state in the given 
     machine */
  if( machine->nuanceSemantics && (machine->nextState != NULL) )
    return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_NEXT_STATE_ALREADY_SET, 
                   NULL, "stateMachine_setNextState: trying to set next state "
                   "to '%s', was previously set to '%s' (Illegal with Nuance "
                   "semantics)", nextState->name, machine->nextState->name );
  else
    machine->nextState = nextState;

  return NULL;
}

Error stateMachine_run( StateMachine machine, StateMachineState initialState )
{
    machine->currentState = initialState;
    machine->nextState = NULL;
    machine->immediateExit = FALSE;
    
    do
    {
      /* 
       * Enter the state. 
       */
        
        /* Call the state class entry func */
        if( (machine->currentState->cls != NULL) && 
            (machine->currentState->cls->entryFunc != NULL) ) {
            
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "<=======\nStatemachine %p: Entering the entry state function %s", 
                machine, machine->currentState->name );
            
            machine->currentState->cls->entryFunc( machine, 
                                                   machine->currentState );
            
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "=======>\nStatemachine %p: Exiting from the entry state function %s", 
                machine, machine->currentState->name );
        }
        
        if( machine->immediateExit )
            break;
        
        /* Call the state entry func */
        if( machine->currentState->entryFunc != NULL ) {
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_INFO )( NULL, "StateMachine",
                LOGLEVEL_INFO, __FILE__,
                __LINE__, 
                "\n<--------------------------------------------------------------\nStatemachine %p: Entering the state function %s", 
                machine, machine->currentState->name );
            
            machine->currentState->entryFunc( machine, machine->currentState );
            
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_INFO )( NULL, "StateMachine",
                LOGLEVEL_INFO, __FILE__,
                __LINE__, 
                "\n-------------------------------------------------------------->\nStatemachine %p: Exiting from the state function %s", 
                machine, machine->currentState->name );
        }
        
        if( machine->immediateExit )
        {
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "Statemachine %p: Performing immediate exit (in state %s).", 
                machine, machine->currentState->name );
            break;
        }
            /* 
            * Leave the state
        */
        /* Call the state entry func */
        if( machine->currentState->exitFunc != NULL ) {
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "<=======\nStatemachine %p: Entering the state post function %s", 
                machine, machine->currentState->name );
            
            machine->currentState->exitFunc( machine, machine->currentState );
            
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "=======>\nStatemachine %p: Exiting from the post function %s", 
                machine, machine->currentState->name );
        }        
        
        if( machine->immediateExit )
        {
            LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
                LOGLEVEL_DEBUG, __FILE__,
                __LINE__, 
                "Statemachine %p: Performing immediate exit (in state %s).", 
                machine, machine->currentState->name );
            break;
        }
        
        if( (machine->currentState->cls != NULL) && 
            (machine->currentState->cls->exitFunc != NULL) )
          machine->currentState->cls->exitFunc( machine, machine->currentState );
        
        /* Go to the next state */
        machine->currentState = machine->nextState;
        machine->nextState = NULL;
    } while( (!machine->immediateExit) && (machine->currentState != NULL) );
    
    LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
        LOGLEVEL_DEBUG, __FILE__,
        __LINE__, 
        "Statemachine %p: Leaving statemachine run.", 
        machine );

    return NULL;
}

Error stateMachine_createAndRun( StateMachineDefinition def, 
                                 StateMachineState initialState, 
                                 void *userData, Boolean nuanceSemantics )
{
  StateMachine machine;
  Error error;

  error = stateMachine_create( def, &machine, userData, nuanceSemantics );
  if( error != NULL )
  {
    error = ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, error, 
      "Failed to create the state machine." );
    return error;
  }

  error = stateMachine_run( machine, initialState );
  if( error != NULL )
    goto CREATE_AND_RUN_FAIL_1;

  error = stateMachine_destroy( machine );

  return error;

CREATE_AND_RUN_FAIL_1:
  stateMachine_destroy( machine ); /* ignore error */

  assert( error != NULL );
  return error;
}

StateMachineState stateMachine_getCurrentState( StateMachine machine )
{
  return machine->currentState;
}

void stateMachine_setImmediateExit( StateMachine machine )
{
    LOG( _voxiUtilGlobalLogLevel >= LOGLEVEL_DEBUG )( NULL, "StateMachine",
        LOGLEVEL_DEBUG, __FILE__,
        __LINE__, 
        "Statemachine %p: Setting the statemachine to exit now!", machine );
    machine->immediateExit = TRUE;
}
