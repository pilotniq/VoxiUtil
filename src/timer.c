/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Voxi's Timer Manager
*/

/*
	#include <stddef.h>
  #include <pthread.h>
	#include <stdio.h>
*/

#ifdef WIN32
#include <windows.h>
#endif


#include <stdlib.h>
#include <assert.h>

#include <voxi/alwaysInclude.h>
#include <voxi/sound/soundStream.h>
#include <voxi/util/threading.h>

#include "timer.h"

CVSID("$Id: timer.c 6184 2003-03-26 15:51:31Z mst $");

/*
 * Macros 
 */

/*******************************************************
 *	Constants
 ********************************************************/

enum TimerType { timerType_oneShot, timerType_repetitive, timerType_random };


/*******************************************************
 *	types
 ********************************************************/

/*
 * Struct to hold an entry in the linked list of timer events.
 */

typedef struct sTimerEvent
{
	int atTimeMillis;
	TimerEventHandlerFunc timerHandler;
	Boolean newThread;
	void *source;
	EventType eventType;
	void *eventData;
	int timerType;
	long repetitiveInterval;
	long randomMinInterval;
	long randomMaxInterval;
  
	TimerEvent prevTE; /* Now a double-linked list for easier removal */
	TimerEvent nextTE;
} sTimerEvent;

/*********************************************************
 *       static function prototypes
 ***********************************************************/

/*
 * The timer callback function
 */
static void timerCallbackFunction();

/*
 * Inserts an event at the correct position in the list.
 * Assumes that the list has been locked
 */
static void insertAnEvent(TimerEvent aEvent);

/*
 * Calculates a random value between min and max.
 *
 */
static long calcRandomInterval(long min, long max);

static void doOneEvent( TimerEvent event );
static void *eventThreadFunc( TimerEvent event );
static long sleep_wakeHandler( long theTime, long nextTime, void *source, 
                               EventType eventType, void *eventData );


/*********************************************************
 *	static data
 **********************************************************/

/*
 * Pointer to the first timer event in the linked list.
 */
static TimerEvent firstTE;

/*
 * Semaphore to protect the linked list
 */
static sVoxiMutex listLock;

static SoundDevice timerDevice;

/*
 * Save the interval between calls in millis
 *
 * Erl changed this from long to int. Surely we won't have more than 32 seconds
 * between timer ticks!
 */
int callbackInterval;

/*
 * Counter that counts millis since start of timer manager
 */
long timerTimeMillis;


/*********************************************************
 *	Startup and shutdown functions
 **********************************************************/

/*
 * Starts the timer event manager
 */
Error tem_startup( SoundDevice timerDev )
{
	Error error = NULL;
	
	firstTE = NULL;

  threading_mutex_init( &listLock );
  
  if( timerDev != NULL )
  {
    /* Register the callback function with the sound stream handler */
    error = soundDev_setTimerCallbackFunc( timerDev, 
                                           timerCallbackFunction, 
                                           &callbackInterval );
    if( error == NULL )
      timerDevice = timerDev;
  }
  
  return error;
}

/*
 * Stops the timer event manager
 */
void tem_shutdown()
{
	TimerEvent aTE, nextTE;
	Error error;
	
	/* Stop callbacking */
	error = soundDev_unsetTimerCallbackFunc( timerDevice );
	assert( error == NULL );
	
	timerDevice = NULL;
	
	/* Lock list and the clean it */
  threading_mutex_lock( &listLock );

	aTE = firstTE;
	while(aTE != NULL)
	{
		nextTE = aTE->nextTE;
		free(aTE);
		aTE = nextTE;
	}

	/* Destroy the semaphore */
	threading_mutex_unlock(&listLock);
	threading_mutex_destroy(&listLock);
}



/*********************************************************
 * Timer callback function
 **********************************************************/


void timerCallbackFunction()
{
	TimerEvent currEvent;
	/* TimerEvent oldEvent; */

#ifndef NDEBUG	
  static int old = 0; /* timerTimeMillis / 1000;*/
#endif
	
	/* Count up the timer */
	timerTimeMillis += callbackInterval;

#ifndef NDEBUG
	if( debug )
		/* Print out seconds */
		if ((timerTimeMillis / 1000) > old)
		{
			/* tem_printTEQ(); */
			printf("%d\n", (int)(timerTimeMillis / 1000));
			old = (timerTimeMillis / 1000);
		}
#endif
	
	/* Check if there are any events to take care of */
	threading_mutex_lock(&listLock);
	currEvent = firstTE;

	while ((firstTE != NULL) && (firstTE->atTimeMillis <= timerTimeMillis))
	{
    currEvent = firstTE;
    
    /* remove the event from the linked list until it is ready to be
       reinserted */
    firstTE = currEvent->nextTE;
    
    if( firstTE != NULL )
      firstTE->prevTE = NULL;
    
    if( currEvent->newThread )
    {
      pthread_t theThread;
      int err;
      
      DEBUG( "newThreadEvent" );
      
      err = threading_pthread_create( &theThread, &detachedThreadAttr, 
                            (ThreadFunc) eventThreadFunc, currEvent );
      
      assert( err == 0 );
    }
    else
    {
      DEBUG( "timer event" );
      
      doOneEvent( currEvent );
    }
	}
	/* Unlock list */
	threading_mutex_unlock( &listLock );
}

/*
  the event should not be in the firstTE linked list when this function is 
  called
*/
static void doOneEvent( TimerEvent currEvent )
{
  long handlerResultTime;
  long nextTime;
  
  /* Calc the proposed nextTime depending on what type of timer it is */
  if (currEvent->timerType == timerType_oneShot)
  {
    nextTime = TIMER_STOP;
  }
  else if (currEvent->timerType == timerType_repetitive)
  {
    nextTime = currEvent->atTimeMillis + currEvent->repetitiveInterval;
  }
  else if (currEvent->timerType == timerType_random)
  {
    nextTime = currEvent->atTimeMillis +
      calcRandomInterval(currEvent->randomMinInterval,
                         currEvent->randomMaxInterval);
  } else {
    /* not a valid TimerType! */
    assert(FALSE);
    nextTime = TIMER_STOP;
  }


  /* Call the handler for the event */
  handlerResultTime = currEvent->timerHandler(currEvent->atTimeMillis,
                                              nextTime,
                                              currEvent->source,
                                              currEvent->eventType,
                                              currEvent->eventData);
      
  /* Should we reinsert the event ? */
  if ((handlerResultTime != TIMER_STOP) &&
      (handlerResultTime > timerTimeMillis))
  {
#if 0
    /* Move to the next event, don't free the current since
     * it's being used again. */
    firstTE = currEvent->nextTE;
    
    if( firstTE != NULL )
      firstTE->prevTE = NULL;
#endif
    /* Set the new time for the event and insert it again */
    currEvent->atTimeMillis = handlerResultTime;
    insertAnEvent(currEvent);
    currEvent = firstTE;
  }
  else
  {
#if 0
    /* Get the next event */
    oldEvent = currEvent;
    currEvent = currEvent->nextTE;
    firstTE = currEvent;
    if( firstTE != NULL )
      firstTE->prevTE = NULL;
#endif
    /* Free the old event */
    free(currEvent); /* was oldEvent */
  }
}

static void *eventThreadFunc( TimerEvent event )
{
#ifndef NDEBUG
  int policy, err;
  struct sched_param schedParam;
  
  err = pthread_getschedparam( pthread_self(), &policy, &schedParam );
  assert( err == 0 );
  
  if( policy != SCHED_OTHER )
  {
    fprintf( stderr, "ERROR: timer.c: eventThreadFunc called as FIFO. "
             "resetting.\n" );
    schedParam.sched_priority = 0;
    
    err = pthread_setschedparam( pthread_self(), SCHED_OTHER, &schedParam );
    assert( err == 0 );                 
  }
    
#endif

  doOneEvent( event );
  
  return NULL;
}

/*********************************************************
 *	Get time and resolution functions
 ***********************************************************/

/*
 * Returns the time in ms that has passed since start of
 * timer event processing
 */
unsigned long tem_getTimeSinceStartMillis()
{
	return (unsigned long) timerTimeMillis;
}


/*
 * Returns the resolution in millis of which the timer event
 * manager works.
 */
long tem_getResolutionMillis()
{
	return callbackInterval;
}



/*********************************************************
 *	Create one-shot timer functions
 ***********************************************************/

/*
 * Schedule a one-shot callback at the absolute time absTimeMillis.
 * timerHandler is the function to be called back to,
 * source is the entity who added the timer, eventtype is the eventtype,
 * eventData is data that should be passed to the event handler func,
 * and execInOwnThread tells the timer manager whether to execute the
 * callback in a thread of its own.
 * Do not call from within a timer handler's thread.
 *
 * If the requested time has already passed, then event is still added to the
 * queue and will be triggered at the next possible opportunity.
 *
 * Note that the returned aEvent will not be valid after the event has been
 * triggered.
 */
TimerEvent tem_addOneShotAbsolute(long absTimeMillis,
																	TimerEventHandlerFunc timerHandler,
																	void *source, EventType eventType,
																	void *eventData, Boolean newThread )
{
	TimerEvent aEvent;
	
	/* a NULL timerHandler is not legal, is it? */
	assert( timerHandler != NULL );
#ifndef NDEBUG
	/* Check so that the time hasn´t passed yet */
	if( absTimeMillis < timerTimeMillis )
	{
		fprintf( stderr, "WARNING: timer event scheduled for the past "
						 "(timer time = %ld, current time = %ld.\n", 
						 timerTimeMillis, absTimeMillis );
	}
#endif
	/* Create the event and set the variables */
	aEvent = malloc(sizeof(sTimerEvent));
	aEvent->atTimeMillis = absTimeMillis;
	aEvent->timerHandler = timerHandler;
  aEvent->source = source;
	aEvent->eventType = eventType;
	aEvent->eventData = eventData;
	aEvent->timerType = timerType_oneShot;
	aEvent->repetitiveInterval = 0;
	aEvent->randomMaxInterval = 0;
	aEvent->randomMinInterval = 0;
	aEvent->newThread = newThread;
  
#ifndef NDEBUG
	if( debug )
		fprintf( stderr, "tem_addOneShotAbsolute at t=%ld, type=%d\n", absTimeMillis, eventType );
#endif

/* Add the event to the queue, first lock then find position */
	threading_mutex_lock( &listLock );

	/* Insert the event */
	insertAnEvent(aEvent);
	
	/* Unlock list */
	threading_mutex_unlock( &listLock );
	
	return aEvent;
}


/*
 * Schedule a one-shot callback at relTimeMillis from the
 * time of calling the function.
 * The rest of the parameters are as above.
 * Do not call from within a timer handler's thread.
 */
TimerEvent tem_addOneShotRelative(long relTimeMillis,
																	TimerEventHandlerFunc timerHandler,
																	void *source, EventType eventType, 
																	void *eventData, Boolean newThread )
{
	return tem_addOneShotAbsolute(timerTimeMillis + relTimeMillis,
																timerHandler, source, eventType,
																eventData, newThread );
}


/*********************************************************
 *	Repetitive and random add funtions
 ***********************************************************/


/*
 * Add a repetitive timer with the interval intervalMillis.
 * The rest of the parameters are as above.
 * The first event occurs on current time + intervalMillis.
 * If the intervalMillis is less than the resolution of with which
 * the timer manager works, then the interval will be set to the
 * resoultion.
 * Do not call from within a timer handler's thread.
 *
 * This function should be modified to return a TimerEvent
 *
 */
void tem_addRepetitive(long intervalMillis,
											 TimerEventHandlerFunc timerHandler,
											 void *source, EventType eventType, void *eventData,
                       Boolean newThread )
{
	TimerEvent aEvent;
	
	/* Check so that the handler is not null */
	if (timerHandler == NULL)
	{
		/* Nothing to do */
		return;
	}

	/* Check the value of interval */
	if (intervalMillis < callbackInterval)
	{
		intervalMillis = callbackInterval;
	}

	/* Create the event and set the variables */
	aEvent = malloc(sizeof(sTimerEvent));
	aEvent->atTimeMillis = timerTimeMillis + intervalMillis;
	aEvent->timerHandler = timerHandler;
  aEvent->source = source;
	aEvent->eventType = eventType;
	aEvent->eventData = eventData;
	aEvent->timerType = timerType_repetitive;
	aEvent->repetitiveInterval = intervalMillis;
	aEvent->randomMaxInterval = 0;
	aEvent->randomMinInterval = 0;
  aEvent->newThread = newThread;
  
	/* Lock the list */
	threading_mutex_lock( &listLock );

	/* Insert the event */
	insertAnEvent(aEvent);

	/* Unlock the list */
	threading_mutex_unlock( &listLock );
}


/*
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
									 void *eventData, Boolean newThread )
{
	TimerEvent aEvent;
	
	/* Check so that the handler is not null */
	if (timerHandler == NULL)
	{
		/* Nothing to do */
		return;
	}

	/* Check the value of the intervals */
	if (minIntervalMillis < callbackInterval)
	{
		minIntervalMillis = callbackInterval;
	}
	if (maxIntervalMillis < callbackInterval)
	{
		maxIntervalMillis = callbackInterval;
	}

	/* Create the event and set the variables */
	aEvent = malloc(sizeof(sTimerEvent));
	aEvent->atTimeMillis =
		timerTimeMillis +	calcRandomInterval(minIntervalMillis,
																				 maxIntervalMillis);
	aEvent->timerHandler = timerHandler;
  aEvent->source = source;
	aEvent->eventType = eventType;
	aEvent->eventData = eventData;
	aEvent->timerType = timerType_random;
	aEvent->repetitiveInterval = 0;
	aEvent->randomMinInterval = minIntervalMillis;
	aEvent->randomMaxInterval = maxIntervalMillis;
  aEvent->newThread = newThread;
  
	/* Lock the list */
	threading_mutex_lock( &listLock );

	/* Insert the event */
	insertAnEvent(aEvent);

	/* Unlock the list */
	threading_mutex_unlock( &listLock );
}


/*
 * Calculates a random value between min and max.
 */
static long calcRandomInterval(long min, long max)
{
#ifndef WIN32
	return min + (random() % (max-min));
#else
  /* XXX: Is rand() really a good replacement for random()? */
	return min + (rand() % (max-min));
#endif
}


/*
 * Cancel (remove) the event
 */
void tem_cancel( TimerEvent event )
{
	/* Lock the list */
	threading_mutex_lock( &listLock );
	
	if( event->prevTE == NULL )
		firstTE = event->nextTE;
	else
		event->prevTE->nextTE = event->nextTE;
	
	if( event->nextTE != NULL )
		event->nextTE->prevTE = event->prevTE;
	
	free( event );
	
	/* Unlock the list */
	threading_mutex_unlock( &listLock );
}

/*********************************************************
 *	Insert and print queue function
 ***********************************************************/

/*
 * Inserts an envent at the correct position in the list.
 * Assumes that the list has been locked
 */
static void insertAnEvent(TimerEvent aEvent)
{
	TimerEvent searchEventPos;
	TimerEvent prevSearchEventPos;

	long absTimeMillis = aEvent->atTimeMillis;

	/* No other events... */
	if (firstTE == NULL)
	{
		firstTE = aEvent;
		aEvent->nextTE = NULL;
		aEvent->prevTE = NULL;
	}
  /*  ...or if to put in first position... */
	else if (firstTE->atTimeMillis >= absTimeMillis)
	{
		aEvent->nextTE = firstTE;
		aEvent->prevTE = NULL;
		
		if( firstTE != NULL )
			firstTE->prevTE = aEvent;
		
		firstTE = aEvent;
	}
	/* ...else search the proper position. */
	else
	{
		searchEventPos = firstTE->nextTE;
		prevSearchEventPos = firstTE;
		
		while ((searchEventPos != NULL) &&
					 (searchEventPos->atTimeMillis < absTimeMillis))
		{
			prevSearchEventPos = searchEventPos;
			searchEventPos = searchEventPos->nextTE;
		}
		
		/* Insert the new event */
		if( prevSearchEventPos->nextTE != NULL )
			prevSearchEventPos->nextTE->prevTE = aEvent;
		
		prevSearchEventPos->nextTE = aEvent;
		aEvent->nextTE = searchEventPos;
		aEvent->prevTE = prevSearchEventPos;
	}
#ifndef NDEBUG
	if( debug > 1 )
		tem_printTEQ();
#endif
}



/*
 * Print the current timer event queue
 */
void tem_printTEQ()
{
#ifndef NDEBUG
	TimerEvent aTE;

	printf("Timer event queue\n");
	aTE = firstTE;
	while (aTE != NULL)
	{
		printf("Event: Time: %d, ET: %d, TT: %d\n", aTE->atTimeMillis, aTE->eventType, aTE->timerType);
		aTE = aTE->nextTE;
	}
#endif
}
	
/*
 *  sleep the current thread for a given number of ms
 */ 
void tem_sleep( int ms )
{
  sem_t semaphore;
  
  sem_init( &semaphore, 0, 0 );
  
  tem_addOneShotRelative( ms, sleep_wakeHandler, &semaphore, 0, NULL, FALSE );
  
  sem_wait( &semaphore );
  
  sem_destroy( &semaphore );
}

static long sleep_wakeHandler( long theTime, long nextTime, void *source, 
                              EventType eventType, void *eventData )
{
  sem_post( (sem_t *) source );
  
  return nextTime;
}
