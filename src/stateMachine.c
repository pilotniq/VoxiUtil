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

typedef struct sStateMachine 
{
  Bag classes;
  Bag states;
  StateMachineState currentState;
  void *userData;
  StateMachineState nextState;
} sStateMachine;

typedef struct sStateClass 
{
  char *name;
  StateMachine machine;
  StateFunction entryFunc;
  StateFunction exitFunc;
  int stateCount;
  void *userData;
} sStateClass;

typedef struct sStateMachineState
{
  char *name;
  StateMachine machine;
  StateClass cls;
  StateFunction entryFunc, exitFunc;
} sStateMachineState;


Error stateMachine_create( StateMachine *machine, void *userData )
{
  Error error;

  error = emalloc( (void **) machine, sizeof( sStateMachine ) );
  if( error != NULL )
    return error;

  (*machine)->classes = bagCreate( 32, 32, NULL );
  (*machine)->states = bagCreate( 32, 32, NULL );
  (*machine)->currentState = NULL;
  (*machine)->userData = userData;
  (*machine)->nextState = NULL;

  return NULL;
}

Error stateMachine_destroy( StateMachine machine )
{
  Error error;

  /* Destroy all states */
  while( bagNoElements( machine->states) > 0 )
  {
    error = stateMachine_destroyState( 
                  (StateMachineState) bagGetRandomElement( machine->states ) );
    if( error != NULL )
      return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, error, 
                     "stateMachine_destroyState failed." );
  }

  /* Destroy all states */
  while( bagNoElements( machine->classes) > 0 )
  {
    error = stateMachine_destroyClass( (StateClass) bagGetRandomElement( machine->classes ) );
    if( error != NULL )
      return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, error, "stateMachine_destroyClass failed." );
  }

  bagDestroy( machine->states, NULL );
  bagDestroy( machine->classes, NULL );
  
  free( machine );
  
  return NULL;
}

Error stateMachine_createClass( StateMachine machine, StateClass *result, 
                                const char *name, 
                                StateFunction entryFunc, StateFunction exitFunc,
                                void *userData )
{
  Error error;

  error = emalloc( (void **) result, sizeof( sStateClass ) );
  if( error != NULL )
    return error;

  (*result)->name = strdup( name );
  (*result)->machine = machine;
  (*result)->entryFunc = entryFunc;
  (*result)->exitFunc = exitFunc;
  (*result)->stateCount = 0;
  (*result)->userData = userData;

  bagAdd( machine->classes, *result );

  return NULL;
}

Error stateMachine_destroyClass( StateClass stateClass )
{
  if( stateClass->stateCount != 0 )
    return ErrNew( ERR_STATE_MACHINE, ERR_STATE_MACHINE_UNSPECIFIED, NULL,
      "The state class still had %d states.", stateClass->stateCount );

  bagRemove( stateClass->machine->classes, stateClass );

  free( stateClass->name ); 

  return NULL;
}

Error stateMachine_createState( StateMachine machine, StateMachineState *state, 
                                const char *name, 
                                StateClass stateClass, StateFunction entryFunc, 
                                StateFunction exitFunc )
{
  Error error;

  error = emalloc( (void **) state, sizeof( sStateMachineState ) );
  if( error != NULL )
    return error;

  (*state)->name = strdup( name );
  (*state)->machine = machine;
  (*state)->cls = stateClass;
  (*state)->entryFunc = entryFunc;
  (*state)->exitFunc = exitFunc;

  bagAdd( machine->states, *state );

  if( stateClass != NULL )
    stateClass->stateCount++;

  return NULL;
}

Error stateMachine_destroyState( StateMachineState state )
{
  if( state->cls != NULL )
    state->cls->stateCount--;

  bagRemove( state->machine->states, state );
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


StateMachine stateMachine_stateGetMachine( StateMachineState state )
{
  return state->machine;
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
  machine->nextState = nextState;

  return NULL;
}

Error stateMachine_run( StateMachine machine, StateMachineState initialState )
{
  machine->currentState = initialState;
  machine->nextState = NULL;

  do
  {
    /* 
     * Enter the state. 
     */
    
    /* Call the state class entry func */
    if( (machine->currentState->cls != NULL) && 
        (machine->currentState->cls->entryFunc != NULL) )
      machine->currentState->cls->entryFunc( machine->currentState );

    /* Call the state entry func */
    if( machine->currentState->entryFunc != NULL )
      machine->currentState->entryFunc( machine->currentState );

    /* 
     * Leave the state
     */
      /* Call the state entry func */
    if( machine->currentState->exitFunc != NULL )
      machine->currentState->exitFunc( machine->currentState );
    
    if( (machine->currentState->cls != NULL) && 
        (machine->currentState->cls->exitFunc != NULL) )
      machine->currentState->cls->exitFunc( machine->currentState );

    /* Go to the next state */
    machine->currentState = machine->nextState;
    machine->nextState = NULL;
  } while( machine->currentState != NULL );

  return NULL;
}
