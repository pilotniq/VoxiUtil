/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/**
   @file timer.h

   Voxi's Timer Manager
*/

#ifndef TIMER_H
#define TIMER_H

#include <voxi/sound/soundDevice.h>
#include <voxi/types.h>

#include <voxi/util/err.h>

#include <voxi/util/event.h>


/**
 * Constant for controlling the timers
 */
#define TIMER_STOP -1



/**
 * Prototype for the callback-function of a timer event.
 *
 * The return value is the actually wanted next time for an event to occur
 * (in absolute terms of time).
 * Normally just pass back the nextTime parameter, but if a repetitive
 * or random timer is to be stopped then send back TIMER_STOP instead.
 * This way even one-shot timers can be used to generate multiple
 * events and makes the timer handling very flexible.
 *
 * source, eventType and eventData are as passed in one of the
 * add-methods.
 * 
 * @param theTime is the current time,
 * @param nextTime is the suggested next time (in absoulte terms)
 *        for this event to occur. \n
          nextTime is suggested based on
 *        how the timer was added (for a one shot it will be
 *        TIMER_STOP, for repetitive it will be theTime + the
 *        interval time, and for random timers it will be according 
 *        wo the random parameters.
 * @param source
 * @param eventType
 * @param eventData 
 */
typedef long (*TimerEventHandlerFunc)(long theTime, long nextTime,
																			void *source, EventType eventType,
																			void *eventData);

/**
 * Abstract type. Handle for the Timer event.
 */
typedef struct sTimerEvent *TimerEvent;

/**
 * Starts the timer event manager
 */
Error tem_startup( SoundDevice timerDevice );

/**
 * Stops the timer event manager
 */
void tem_shutdown();


/**
 * Returns the time in ms that has passed since start of
 * timer event processing
 */
unsigned long tem_getTimeSinceStartMillis();

/**
 * Returns the resolution in millis of which the timer event
 * manager works.
 */
long tem_getResolutionMillis();


/**
 * Schedule a one-shot callback at the absolute time absTimeMillis.
 * timerHandler is the function to be called back to,
 * source is the entity who added the timer, eventtype is the eventtype,
 * eventData is data that should be passed to the event handler func.
 * Do not call from within a timer handler's thread.
 *
 * Changed by erl to return a TimerEvent so that the event can be cancelled.
 * This feature is needed by SoundFork.
 *
 * Also changed so that scheduling an event at a time in the past will cause
 * the timerHandler to be called immediately.
 *
 * newThread will cause the timerHandler to be called from a new thread. 
 * This should be the default, unless short latencies are required.
 *   If the timerHandler is not called from a new thread, it is very easy
 * to create deadlocks, since the timerHandler will be called from the same
 * thread as writes the audio data.
 *
 */
TimerEvent tem_addOneShotAbsolute(long absTimeMillis,
																	TimerEventHandlerFunc timerHandler,
																	void *source, EventType eventType,
																	void *eventData, Boolean newThread );


/**
 * Schedule a one-shot callback at relTimeMillis from the
 * time of calling the function.
 * The rest of the parameters are as above.
 * Do not call from within a timer handler's thread.
 */
TimerEvent tem_addOneShotRelative(long relTimeMillis,
																	TimerEventHandlerFunc timerHandler,
																	void *source, EventType eventType,
																	void *eventData, Boolean newThread );

/**
 * Add a repetitive timer with the interval intervalMillis.
 * The rest of the parameters are as above.
 * The first event occurs on current time + intervalMillis.
 * If the intervalMillis is less than the resolution of with which
 * the timer manager works, then the interval will be set to the
 * resoultion.
 * Do not call from within a timer handler's thread.
 */
void tem_addRepetitive(long intervalMillis,
											 TimerEventHandlerFunc timerHandler,
											 void *source, EventType eventType,
											 void *eventData, Boolean newThread);

/**
 * Add a random timer with the minimum interval minIntervalMillis
 * and the maximum interval maxIntervalMillis.
 * The rest of the parameters are as above.
 * The first event occurs no later than in maxIntervalMillis and
 * no sooner than minIntervalMillis.
 * If the any of the intervals is less than the resolution of with which
 * the timer manager works, then the interval will be set to the
 * resoultion.
 * Do not call from within a timer handler's thread.
 */
void tem_addRandom(long minIntervalMillis, long maxIntervalMillis,
									 TimerEventHandlerFunc timerHandler,
									 void *source, EventType eventType,
									 void *eventData, Boolean newThread );

/**
 *  Cancel a timer event so that it will not happen.
 *
 *  @note 
 *    it is important that you do not cancel an event which has happened.
 */
void tem_cancel( TimerEvent timerEvent );

/**
 * Print the current timer event queue. For debugging
 */
void tem_printTEQ();

/**
 * delays the current thread for ms ms
 */
void tem_sleep( int ms );

#endif
