/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
 * @file event/event.h
 *
 * Voxi's Event Manager.
 *
 * @author dad
 *
 * Copyright (C) 1999-2002, Voxi AB
 *
 * This is the old, non-oow event manager, still used
 * by some parts of the system.
 *
 * @warning This event system is deprecated. Further on, you should
 * use the oow-ified event system found in eventType.h, OowEvent.h
 *
 * @see event.h
 * @see OowEvent.h
 *
 * EventType is an indentifier for a type of event.
 *
 * Below is a short list of event ranges:
 *   Sound:  256 - 511 \n
 *   Text Rendering:   512 - 639 \n
 *   Sound rendering:   640 - 767 \n
 *   Sound Fork:        768 - 895 \n
 *   Speech detector:   898 - 1023 \n
 *   Speech recognizer: 1024- 1279 \n
 *   Object movement:   1280- 1535 \n
 *   Language rendering: 1536-1700 \n
 *   Game types:        32768- \n
 */

#ifndef EVENT_H
#define EVENT_H

/**
 * For values in use, see above.
 */
typedef int EventType;

#include <voxi/types.h>

/* This is because on Windows, when somebody else uses this as a DLL,
   we must declare external variables with a special directive.
   Note: Since the EXTERN macro may be redefined in other .h files, the
   following macro sequence must occur after any other inclusion
   made in this .h file. */
#ifdef EXTERN
#  undef EXTERN
#endif
#ifdef LIB_EVENT_INTERNAL
#  define EXTERN extern
#else
#  ifdef WIN32
#    define EXTERN __declspec(dllimport)
#  else
#    define EXTERN extern
#  endif /* WIN32 */
#endif /* LIB_EVENT_INTERNAL */



/*Ä
 * Prototype for the callback-function that an event listener
 * wants to get called when an event occurs.
 */
typedef void (*EventHandlerFunc)( void *source, EventType eventType, 
																	void *eventData, void *handlerData );

/**
 * Prototype for the callback-function that an event sender
 * wants to get called when an event has been completely
 * handled. The metgod may for example clean up the event data
 * that got passed in the event.
 */
typedef void (*EventFreeFunc)( void *source, EventType eventType, 
															 void *eventData );

/*
 * Global variables
 */

/*
 * Start the event handler.
 */
void em_startup();

/**
 * Ends the event handling and frees the resources that it used.
 * There might be spawned  eventhandling threads running after
 * this function returns...
 */
void em_shutdown();

/**
 * Post an event.
 * source and eventType may not be null.
 *
 * Set freeFunc to an EventFreeFunc-function to get a callback when
 * the event has been completely handled to for example clean
 * up the event data.
 *
 * If blocking is set, the the function call will not return until
 * all listeners have been called and have returned, and the freeFunc
 * (if any) has been called.
 */
void em_postEvent( void *source, EventType eventType, void *eventData, 
                   EventFreeFunc freeFunc, Boolean blocking );

/**
 * Add an event listener.
 * The listener will be called when an event with a matching
 * source, eventType and
 */
void em_addListener( void *source, EventType eventType,		     
		     EventHandlerFunc handlerFunc, 
		     void *handlerData, Boolean makeNewThread );

/**
 * Remove an event listener from the specified source.
 */
void em_removeListener( void *source, EventType eventType, 
			EventHandlerFunc handlerFunc, void *handlerData );

#endif

