/*
 * stateMachine.h
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <voxi/util/err.h>

typedef struct sStateMachine *StateMachine;
typedef struct sStateClass *StateClass;
typedef struct sStateMachineState *StateMachineState;

typedef void (*StateFunction)( StateMachineState );

enum { ERR_STATE_MACHINE_UNSPECIFIED };

EXTERN_UTIL Error stateMachine_create( StateMachine *machine, void *userData );
EXTERN_UTIL Error stateMachine_destroy( StateMachine machine );

EXTERN_UTIL Error stateMachine_createClass( StateMachine, StateClass *result, 
                                            const char *name, StateFunction entryFunc, 
                                            StateFunction exitFunc, void *userData );
EXTERN_UTIL Error stateMachine_destroyClass( StateClass stateClass );

EXTERN_UTIL Error stateMachine_createState( StateMachine, StateMachineState *state, 
                                            const char *name, StateClass, 
                                            StateFunction entryFunc, 
                                            StateFunction exitFunc );
EXTERN_UTIL Error stateMachine_destroyState( StateMachineState state );

EXTERN_UTIL void *stateMachine_classGetUserData( StateClass cls );

EXTERN_UTIL StateMachine stateMachine_stateGetMachine( StateMachineState );
EXTERN_UTIL StateClass stateMachine_stateGetClass( StateMachineState state );

EXTERN_UTIL void *stateMachine_classGetUserData( StateClass cls );

EXTERN_UTIL void *stateMachine_getUserData( StateMachine );

EXTERN_UTIL Error stateMachine_setNextState( StateMachine machine, 
                                             StateMachineState nextState );

EXTERN_UTIL Error stateMachine_run( StateMachine machine, StateMachineState initialState );

#ifdef __cplusplus
}
#endif

#endif