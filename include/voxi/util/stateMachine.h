/*
 * stateMachine.h
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <voxi/util/config.h>
#include <voxi/util/err.h>
#include <voxi/util/logging.h>

typedef struct sStateMachine *StateMachine;
typedef struct sStateMachineDefinition *StateMachineDefinition;
typedef struct sStateClass *StateClass;
typedef struct sStateMachineState *StateMachineState;

typedef void (*StateFunction)( StateMachine, StateMachineState );


enum { ERR_STATE_MACHINE_UNSPECIFIED, ERR_STATE_MACHINE_OUT_OF_MEMORY, 
       ERR_STATE_MACHINE_DEF_HAS_MACHINES, 
       ERR_STATE_MACHINE_NEXT_STATE_ALREADY_SET };

EXTERN_UTIL Error stateMachine_defCreate( StateMachineDefinition *machineDef );
EXTERN_UTIL Error stateMachine_defDestroy( StateMachineDefinition machineDef );

/*
 * nuanceSemantics means that the next state may only be set once
 */
EXTERN_UTIL Error stateMachine_create( StateMachineDefinition def, 
                                       StateMachine *machine, void *userData,
                                       Boolean nuanceSemantics );
EXTERN_UTIL Error stateMachine_destroy( StateMachine machine );

EXTERN_UTIL Error stateMachine_createClass( StateMachineDefinition, StateClass *result, 
                                            const char *name, StateFunction entryFunc, 
                                            StateFunction exitFunc, void *userData );
EXTERN_UTIL Error stateMachine_destroyClass( StateClass stateClass );

EXTERN_UTIL Error stateMachine_createState( StateMachineDefinition, StateMachineState *state, 
                                            const char *name, StateClass, 
                                            StateFunction entryFunc, 
                                            StateFunction exitFunc );
EXTERN_UTIL Error stateMachine_destroyState( StateMachineState state );

EXTERN_UTIL void *stateMachine_classGetUserData( StateClass cls );

EXTERN_UTIL StateMachineDefinition stateMachine_stateGetMachineDefinition( StateMachineState );
EXTERN_UTIL StateClass stateMachine_stateGetClass( StateMachineState state );

EXTERN_UTIL void *stateMachine_classGetUserData( StateClass cls );

EXTERN_UTIL void *stateMachine_getUserData( StateMachine );

EXTERN_UTIL Error stateMachine_setNextState( StateMachine machine, 
                                             StateMachineState nextState );
EXTERN_UTIL void stateMachine_setImmediateExit( StateMachine machine );

EXTERN_UTIL StateMachineState stateMachine_getCurrentState( StateMachine machine );

EXTERN_UTIL Error stateMachine_run( StateMachine machine, StateMachineState initialState );
/*
 * See stateMachine_create for meaning of parameters
 */
EXTERN_UTIL Error stateMachine_createAndRun( StateMachineDefinition, 
                                             StateMachineState initialState,
                                             void *userData, 
                                             Boolean nuanceSemantics );

EXTERN_UTIL char *stateMachine_getStateName( StateMachine machine );

#ifdef __cplusplus
}
#endif

#endif