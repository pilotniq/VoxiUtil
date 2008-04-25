/*
  Copyright (C) 1999-2002 Voxi AB. All rights reserved.

  This software is the proprietary information of Voxi AB, Stockholm, Sweden.
  Use of this software is subject to license terms.

*/

/*
  Voxi's Event Manager
*/

#ifdef _WIN32_WCE
#include <windows.h>
/* all of a sudden we needed time_t to be defined in time.h that is
   now included in pthread. we also need wchar_t that is also used in
   time.h. Its defined in windows.h that used to be included in
   pthread.h, but not anymore..
*/
#ifdef POCKETCONSOLE
#include <sys/types.h>
#endif
#endif

/* Whether to generate debugging messages or not.
   Should be before alwaysInclude.h */
#define DEBUG_MESSAGES 0
#include <voxi/alwaysInclude.h>

#include <stddef.h>
#include <pthread.h>
#ifdef HAS_SEMAPHORE_H
#include <semaphore.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <voxi/util/win32_glue.h>
#endif /* WIN32 */

#include <voxi/util/hash.h>
#include <voxi/util/vector.h>

#include <voxi/util/threading.h>

#include <voxi/util/event.h>

/* 
   Hacked by erl 2007-08-30
   Couldn't we use the queue class in util, and not need this limitation?
 */
#define EVENT_MAX_EVENTS 256 
   
#ifndef EVENT_MAX_EVENTS
#error You must define EVENT_MAX_EVENTS to be the maximum number of events in the event queue at any time
#endif

#ifdef _WIN32_WCE
/* A hack to not be dependent on oow.lib */
/* this will break event-stuff if oow-objects are used, 
   but by then oow.lib will of course work, so this 
   hack wont be needed -- ara */
#define uriCreate( X, Y, Z, W, B ) NULL
#endif /* _WIN32_WCE */

CVSID("$Id: event.c 6184 2003-03-26 15:51:31Z mst $");


/*******************************************************
 *	types
 ********************************************************/

/*
 * Struct holds an event generated from <source> of the type <type>
 * and with the event data eventData. freeFunc is the function
 * to be called upon completion of handling of this event.
 */
typedef struct
{
  void *source;
  EventType eventType;
  void *eventData;
  EventFreeFunc freeFunc;
  /*
    If the post of the event is blocking, then the poster will wait for the
    semaphore before returning. The semaphore will be triggered after the
    freeFunc is called.
  */
  Boolean isBlocking;
  sem_t semaphore;
} sEvent, *Event;


/*
 * Struct to hold a listener entry in the listener list.
 * handlerFunc and handlerData  make up the keys in the list.
 */
typedef struct
{
  EventHandlerFunc handlerFunc;
  void *handlerData;
  Boolean makeNewThread;
} sListenerEntry, *ListenerEntry;


/*
 * Struct to hold the data for every event listener list
 * in the hashtable. It has to store the two keys as
 * well to make it possible to compare the keys exactly
 * if the hash code should be the same. The listenerList
 * contains entrys of the type ListenerEntry.
 */
#define LLIST_NO_ELEMS 50
typedef struct
{
  void *source;
  EventType eventType;
	sVoxiMutex listenerListLock;
  ListenerEntry listenerEntryList[LLIST_NO_ELEMS];
} sListenerList, *ListenerList;


/*
 * Defining the thread start-routine prototype
 * to make it possible to cast to it
 */
typedef void * (*thread_start_routine) (void *);

/*
 * Struct to hold the data that an event handlerfunc needs.
 * this is to be able to pass it all with a single pointer
 * as needed by the thread_start_routine prototype
 */
typedef struct
{
	EventHandlerFunc handlerFunc;
	void *source;
	EventType eventType;
	void *eventData;
	void *handlerData;
} sHandleOneListenerData, *HandleOneListenerData;



/*********************************************************
 *       static function prototypes
 ***********************************************************/

/* Event handler functions */
static void *em_mainLoop( void *args );
static void *em_handleOneEvent( Event event );
static void *em_handleOneListenerHandlerFunc(HandleOneListenerData data);

/* internal Event functions */
static void finishedWithEvent( Event event );
static void freeEvent( Event event );

/* Hashtable handling routines */
static int calcHashCode(ListenerList aList);
static int compHashEntrys(ListenerList list1, ListenerList list2);
static void freeListenerList(ListenerList aList);
static void writeListenerList(ListenerList aList);


/*
 * global data
 */

/*********************************************************
 *	static data
 ***********************************************************/

/*
 * Semaphores for the event queue counter and modification lock.
 */
static sem_t eventsInQueue;
static sVoxiMutex eventQueueModificationLock;


/*
  Store the pending events
  frontEvent points to the earliest event in the queue. This is the next
  event to be handled.	 
  The event after frontEvent is at (frontEvent + 1) % EVENT_MAX_EVENTS
  backEvent points to where to add the next event
  if frontEvent == backEvent then the queue is empty
*/
static int frontEvent = 0, backEvent = 0;
static Event eventQueue[ EVENT_MAX_EVENTS ];


/*
 * The event manager thread
 */
static pthread_t eventManagerThread = (pthread_t)NULL;


/*
 * The hashtable that containts the added listeners.
 * Actually it contains a ListenerList for every
 * entry with the same keys (i.e. source and eventType) 
 */
static HashTable listenersHashTable = NULL;
static sVoxiMutex listenersHashTableLock;


/*********************************************************
 *	start/stop functions
 ***********************************************************/

/*
 * Start the event handler.
 * Initializes the Hashtable, event queue, semaphores and starts
 * the main event manager thread.
 */
void em_startup()
{
  int err;
  Error error;
    
  /* Init the the listnersHashTable */
  listenersHashTable =
    HashCreateTable(1024,
		    (HashFuncPtr)calcHashCode, 
		    (CompFuncPtr)compHashEntrys, 
		    (DestroyFuncPtr) freeListenerList);
  
  threading_mutex_init( &(listenersHashTableLock) );
  
  /* Init the event handler thread */
	assert( eventManagerThread == (pthread_t) NULL );
  
  frontEvent = 0;
  backEvent = 0;
  
  err = sem_init( &eventsInQueue, 0, 0 );
  assert( err == 0 );
  
  threading_mutex_init( &eventQueueModificationLock );
  assert( err == 0 );
  
	/* So that we can access teh detachedThreadAttr variable */
	threading_init();
	
	/* The Event Manager Thread will be joined upon shutdown, and is therefore
		 not created in a detached state */
	
  err = threading_pthread_create( &eventManagerThread, NULL, em_mainLoop, NULL );
  assert( err == 0 );
}


/*
 * Ends the event handling and frees the resources that it used.
 * There might be spawned  eventhandling threads running after
 * this function returns...
 */
void em_shutdown()
{
	int error;
	
  /* Stop the event handler */
	error = pthread_cancel(eventManagerThread);
	assert(error == 0);
	error = pthread_join(eventManagerThread, NULL);
	assert(error == 0);
  
	threading_shutdown();
	
  /* skip destroying the hash table for now.
     
     Destroying the hash table requires the hash table to be empty. But doing
     that requires all objects to be shut down properly, which we aren't doing
     yet.
  */
#if 0 
  /* Destroy the hash table */
  HashDestroyTable(listenersHashTable); 	
#endif
  /* Destroy semaphores */
	error = sem_destroy(&eventsInQueue);
	assert(error == 0);
  
	threading_mutex_destroy( &eventQueueModificationLock );
}


/*********************************************************
 *	Event sending function
 ***********************************************************/

/*
 * Post an event.
 * Source may not be null.
 *
 */
void em_postEvent( void *source, EventType eventType, void *eventData, 
                   EventFreeFunc freeFunc, Boolean blocking )
{
	int error = 0;
	Event aEvent = NULL;

	DEBUG(" enter ( source %p, type %d, eventData %p, freeFunc %p, blocking %d )\n",
        source, eventType, eventData, freeFunc, blocking );
	
	assert (source != NULL);
	
	/* Check if there are free space in the event queue */
	assert( ((backEvent + 1) % EVENT_MAX_EVENTS) != frontEvent);
	
  DEBUG("before aquiring queue-lock\n" );
	
	/* Acquire the lock to the event list */
	threading_mutex_lock( &eventQueueModificationLock );
	assert( error == 0 );
 
  DEBUG("after  aquiring queue-lock\n" );
	
	/* Create new event and insert the event at the point of backEvent */
	aEvent = malloc(sizeof(sEvent));
	aEvent->source = source;
	aEvent->eventType = eventType;
	aEvent->eventData = eventData;
	aEvent->freeFunc = freeFunc;
  aEvent->isBlocking = blocking;
  if( blocking )
    sem_init( &(aEvent->semaphore), 0, 0 );
  
	eventQueue[backEvent] = aEvent;

	/* Modify the backEvent pointer */
	backEvent = (backEvent + 1) % EVENT_MAX_EVENTS;

	/* Release the event list */
	threading_mutex_unlock( &eventQueueModificationLock );
	
	/* Increase the QueueCounter by 1 to indicate a new event */
  DEBUG(" sem_post(&eventsInQueue)\n" );
	error = sem_post(&eventsInQueue);
	assert( error == 0 );
	
  if( blocking )
  {
    int err;
    
		DEBUG("befor sem_wait(aEvent=%p)\n", aEvent );
    err = threading_sem_wait( &(aEvent->semaphore) );
    assert( err == 0 ); 
		DEBUG("after sem_wait(aEvent=%p)\n", aEvent );

    freeEvent( aEvent );
  }
  
  DEBUG("leave\n");
}



/*********************************************************
 *	Add and remove listener functions
 ***********************************************************/


/*
 * Adds a listener.
 * First it finds the listenerList from the hash table (or creates
 * a new if not found) with source and eventType as keys.
 * Then a entry is insterted into the list with handlerFunc and
 * handlerData as keys
 */
void em_addListener( void *source, EventType eventType, 
		     EventHandlerFunc handlerFunc, 
		     void *handlerData, Boolean makeNewThread )
{
  int i;
  int res;
  /* The soon enough found listenerlist */
  ListenerList listenerList = NULL;
  /* The template of how to find the listenerList */
  sListenerList findTemplate = {source, eventType, {{0}}, {NULL} };

	DEBUG(" enter ( source %p, type %d, handlerData %p, handlerFunc %p, makeNewThread %d )\n",
        source, eventType, handlerData, handlerFunc, makeNewThread );
  
  threading_mutex_lock( &(listenersHashTableLock) );
  
  /* Get listener list from hash table with keys (source, eventType) */
  listenerList = HashFind(listenersHashTable, &findTemplate);

  /* Check if we got a listenerList or if we got null */
  if (listenerList == NULL)
  {
    DEBUG("create new listenersList for {source %p, type %d}\n", source, eventType);
    /* If null then create a new listenerList, initialize
     * it and add it to the Hashtable */
    listenerList = malloc(sizeof(sListenerList));
    listenerList->source = source;
    listenerList->eventType = eventType;
    
    threading_mutex_init( &(listenerList->listenerListLock) );
    
		/* error = sem_init( &(listenerList->listenerListLock), 0, 1 );
       assert( error == 0 ); */
    
    for (i = 0; i < LLIST_NO_ELEMS; i++)
      listenerList->listenerEntryList[i] = NULL;
    res = HashAdd(listenersHashTable, listenerList);
    /* Check if success */
    assert(res != 0);
  }
  
  threading_mutex_unlock( &(listenersHashTableLock) );
  
  /* Now we got a listenerList. First lock it.
   * Then loop through the list to check for an empty entry */
  
  DEBUG(" get      lock %p->listenerListLock \n", listenerList );
  threading_mutex_lock( &(listenerList->listenerListLock) );
  DEBUG(" got      lock %p->listenerListLock \n", listenerList );
  
	for (i=0; i < LLIST_NO_ELEMS; i++)
  {
    if (listenerList->listenerEntryList[i] == NULL)
    {
      /* Add it here */
      listenerList->listenerEntryList[i] = 
				(ListenerEntry) malloc(sizeof(sListenerEntry));
      listenerList->listenerEntryList[i]->handlerFunc = handlerFunc;
      listenerList->listenerEntryList[i]->handlerData = handlerData;
      listenerList->listenerEntryList[i]->makeNewThread = makeNewThread;
			
      break;
    }
  }
	/* Unlock list */
  threading_mutex_unlock( &(listenerList->listenerListLock) );
  DEBUG(" released lock %p->listenerListLock \n", listenerList );
	/* sem_post(&(listenerList->listenerListLock)); */

  /* Error if no free entrys in list */
	assert (i < LLIST_NO_ELEMS);

#ifndef NDEBUG
	if( debug )
	{
		fprintf( stderr, "Added listener ");
		writeListenerList(listenerList);
	}
#endif /* NDEBUG */

  DEBUG("leave\n");
}



/*
 * Removes the first occurrance of listener with the keys
 * handlerFunc and handlerData from a list retrieved from the
 * hash table with the keys source and eventType.
 */
void em_removeListener( void *source, EventType eventType, 
			EventHandlerFunc handlerFunc, void *handlerData )
{
  int i;
  /* The soon enough found listenerlist */
  ListenerList listenerList = NULL;
  /* The template of how to find the listenerList */
  sListenerList findTemplate = {source, eventType, {{0}}, {NULL} };

	DEBUG(" enter ( source %p, type %d, handlerData %p, handlerFunc %p )\n",
        source, eventType, handlerData, handlerFunc );

  threading_mutex_lock( &(listenersHashTableLock) );
  
  /* Get listener list from hash table with keys (source, eventType) */
  listenerList = HashFind(listenersHashTable, &findTemplate);

  threading_mutex_unlock( &(listenersHashTableLock) );
  
  assert( listenerList != NULL );
  
	/* Lock the list */
  DEBUG(" get      lock %p->listenerListLock \n", listenerList );
	threading_mutex_lock(&(listenerList->listenerListLock));
  DEBUG(" got      lock %p->listenerListLock \n", listenerList );

  /* Loop through the list to check for the entry with
   * the keys handlerFund and handlerData */
  for (i=0; i < LLIST_NO_ELEMS; i++)
  {
    if ((listenerList->listenerEntryList[i] != NULL) &&
	(listenerList->listenerEntryList[i]->handlerFunc == handlerFunc) &&
	(listenerList->listenerEntryList[i]->handlerData == handlerData))
    {
      /* Found a match, remove it */
      free(listenerList->listenerEntryList[i]);
      listenerList->listenerEntryList[i] = NULL;
			DEBUG("removed listenerEntry\n");

      break;
    }
  }

	/* Unlock list */
	threading_mutex_unlock(&(listenerList->listenerListLock));
  DEBUG(" released lock %p->listenerListLock \n", listenerList );
	
#ifndef NDEBUG
	if( debug )
	{
		printf("Removing listener  ");
		writeListenerList(listenerList);
	}
#endif
  DEBUG("leave\n");
}




/*********************************************************
 *	Internal event dispatch functions
 ***********************************************************/

/*
 * The main event manager loop.
 * Waits for the queue to contain at least one event and starts
 * to take it off the queue. It then starts a new thread to
 * handle that current event.
 */
static void *em_mainLoop( void *args )
{
  int error;
  
  while( TRUE )
  {
    Event event;
    pthread_t oneEventHandlerThread;

    /* Wait for some events to be added 
		 * This is also the cancelation point for this thread */

    DEBUG("waits for events\n");
    error = threading_sem_wait( &eventsInQueue );
    assert( error == 0 );
    /*
    if( error != 0 )
      fprintf( stderr, "event.c: catastrophic: sem_wait returned %d, not 0. errno=%d\n",
      error, errno );
    */
    assert( frontEvent != backEvent );

    /* get and remove the element from the queue */
    event = eventQueue[ frontEvent ];
    
    threading_mutex_lock( &eventQueueModificationLock );

		/* modify the queue */
    frontEvent = (frontEvent + 1) % EVENT_MAX_EVENTS;
    
    threading_mutex_unlock( &eventQueueModificationLock );

    /* Create a thread that dispatches the callbacks to the
     * registred listeners */

    error = threading_pthread_create( &oneEventHandlerThread, &detachedThreadAttr,
                            (thread_start_routine) em_handleOneEvent, event );
    assert( error == 0 );
    
    DEBUG(" forked (detached) thread %ld for event %p\n", 
          oneEventHandlerThread, event);
    
#ifndef NDEBUG
    {
    int scheduler;
    struct sched_param schedParam;
    
    error = pthread_getschedparam( pthread_self(), &scheduler, &schedParam );
    assert( error == 0 );
    
    if( scheduler != SCHED_OTHER )
      fprintf( stderr, "event.c: trouble, scheduler == %d != SCHED_OTHER == %d.\n", scheduler, SCHED_OTHER );
    
    /* assert( scheduler == SCHED_OTHER ); */
    }
#endif
  }
  
  
  return NULL;
}

static void freeEvent( Event event )
{
  if( event->isBlocking )
  {
    int err;
    
    /* free the semaphore */
    err = sem_destroy( &(event->semaphore) );
    assert( err == 0 );
    DEBUG(" destroyed %p->semaphore\n",event);
  }
  
  free( event );
}

/*
  posts to the event's semaphore if the event is blocking, otherwise frees it.
  
  If it is a blocking event, then the thread that does sem_wait must also
  do the freeing by calling freeEvent.
*/
static void finishedWithEvent( Event event )
{
  /* free the event */
  if (event->freeFunc)
    event->freeFunc(event->source, event->eventType, event->eventData);
  
  if( event->isBlocking )
  {
    int err;
  
    DEBUG(" sem_post %p->semaphore\n",event);
    err = sem_post( &(event->semaphore ) );
    assert( err == 0 );
    /* freeEvent will be called from em_postEvent */
  }
  else
    freeEvent( event );
}

/*
 * Handles the execution of a single event.
 * It first fetches the list of all listeners for
 * the event's source and eventType.
 * For every listener, the handlerFunction is called
 * either within a new thread or within the current thread
 * depending on what the variable makeNewThread is
 */
static void *em_handleOneEvent(Event event)
{
	int listnCntr = 0;
  
	/* List with threads to be checked for completion */
	pthread_t threadWaitList[LLIST_NO_ELEMS];
	int threadWaitCntr = 0;
		
  /* The soon enough found listenerlist */
  ListenerList listenerList = NULL;
  /* The template of how to find the listenerList initiated
	 * with the event's source and eventType */
  sListenerList findTemplate = {event->source, event->eventType, {{0}}, {NULL} };
  
  DEBUG(" enter %p\n", event);
  
  /* Check if we have erraneously gotten realtime scheduling */
#ifndef NDEBUG
  {
  int err, scheduler;
  struct sched_param schedParam;
  err = pthread_getschedparam( pthread_self(), &scheduler, &schedParam );
  assert( err == 0 );
    
  assert( scheduler == SCHED_OTHER );
  }
#endif

  
  threading_mutex_lock( &(listenersHashTableLock) );
  
  /* Get listener list from hash table with keys (source, eventType) */
  listenerList = HashFind(listenersHashTable, &findTemplate);

  threading_mutex_unlock( &(listenersHashTableLock) );
  
  /* Check if we got a listenerList or if we got null */
  if (listenerList == NULL)
  {
		/* No listeners... */
    DEBUG("No listeners\n");

    finishedWithEvent( event );
#if 0
    /* free the event */
    if (event->freeFunc)
      event->freeFunc(event->source, event->eventType, event->eventData);
    
    free( event );
#endif
    DEBUG("leave (no listeners)\n");
		return NULL;
	}

	/* Lock the list */
  DEBUG(" get      lock %p->listenerListLock \n", listenerList );
	threading_mutex_lock(&(listenerList->listenerListLock));
  DEBUG(" got      lock %p->listenerListLock \n", listenerList );

	/* Loop through the list and for each non-null position
	 * call the handlerFunc in the desired way */
  for (listnCntr=0; listnCntr < LLIST_NO_ELEMS; listnCntr++)
  {
		/* Check if there is a listener */
    if (listenerList->listenerEntryList[listnCntr] != NULL)
    {
			ListenerEntry aListener = NULL;
			aListener = listenerList->listenerEntryList[listnCntr];

			/* Check if a new thread or within in this thread */
			if (aListener->makeNewThread)
			{
				int error = 0;
				HandleOneListenerData aHandleOneListenerData = NULL;
				pthread_t oneEventListenerThread;
				
				/* Ok, in a new thread, first build the data struct */
				aHandleOneListenerData = malloc(sizeof(sHandleOneListenerData));
				aHandleOneListenerData->handlerFunc = aListener->handlerFunc;
				aHandleOneListenerData->source = event->source;
				aHandleOneListenerData->eventType = event->eventType;
				aHandleOneListenerData->eventData = event->eventData;
				aHandleOneListenerData->handlerData = aListener->handlerData;
#ifndef WIN32				
        if( sched_getscheduler( getpid() ) == SCHED_FIFO )
          fprintf( stderr, "em_handleOneEvent with scheduler FIFO.\n" );
#endif
  
				/* Execute listeners handlerFunc in its own thread */
        DEBUG("forks thread for event %p  handlerData %p ... \n", 
              event, aListener->handlerData);

				error = threading_pthread_create( &oneEventListenerThread, NULL,
											 (thread_start_routine) em_handleOneListenerHandlerFunc,
																aHandleOneListenerData);

        DEBUG("   ... thread created %ld\n", oneEventListenerThread ); 

        if( error == 0 )
        {
          /* Add thread to wait list */
          assert( threadWaitCntr < LLIST_NO_ELEMS );
          threadWaitList[threadWaitCntr] = oneEventListenerThread;
          threadWaitCntr++;
        }
        else
          fprintf( stderr, "ERROR: error %d starting event thread.\n",
                   error );
			}
			else
			{
        /* This is a dangerous point. If the handlerFunc blocks for a long
           time, we still have the listenerList locked, and will prevent other
           events like this to be handled. 
             Any handlers which can block should start new threads! 
        */
        DEBUG("forks NO thread for event %p  handlerData %p\n",
              event, aListener->handlerData);

				aListener->handlerFunc(event->source, event->eventType,
															event->eventData, aListener->handlerData);

        DEBUG("handlerFunc returned-1. event %p  handlerData %p\n",
              event, aListener->handlerData);
			}
		}
	}

	/* Unlock the list */
	threading_mutex_unlock(&(listenerList->listenerListLock));
  DEBUG(" released lock %p->listenerListLock \n", listenerList );

	/* Wait for all threads in wait list */
  DEBUG(" waits for joining threads for event %p\n", event );
	for (listnCntr = 0; listnCntr < threadWaitCntr; listnCntr++)
	{
		/* For every pthread created, do a join */
		pthread_join(threadWaitList[listnCntr], NULL);
	}
  DEBUG(" done joining threads for event %p\n", event );
	
  finishedWithEvent( event );
#if 0
	/* Check if freeFunc should be called. */
	if (event->freeFunc)
	{
		event->freeFunc(event->source, event->eventType, event->eventData);
	}

	/* Free the event itself */
	free(event);
#endif

  DEBUG("leave\n");
  return NULL;
}


/*
 * This function handles a single callback to a listener
 * handleFunc. It's here because the handlerFunc
 * and the thread_start_routine doesn't have the same signature
 */
static void *em_handleOneListenerHandlerFunc(HandleOneListenerData data)
{
#ifndef NDEBUG
  int policy, err;
  struct sched_param schedParam;
  
  err = pthread_getschedparam( pthread_self(), &policy, &schedParam );
  assert( err == 0 );
  
  if( policy != SCHED_OTHER )
  {
    DEBUG( "ERROR: em_handleOneListenerHandlerFunc called as FIFO. "
           "resetting.\n" );
    schedParam.sched_priority = 0;
    
    err = pthread_setschedparam( pthread_self(), SCHED_OTHER, &schedParam );
    assert( err == 0 );                 
  }
    
#endif
  /* Call the handler func */
	data->handlerFunc(data->source, data->eventType,
										data->eventData, data->handlerData);

  DEBUG("handlerFunc returned-2. event ??  handlerData %p\n",
        /*event,*/ data->handlerData);

	/* Free the data */
	free(data);
	
	return NULL;
}





/*********************************************************
 *  Hashtable handling functions
 ***********************************************************/


static int calcHashCode(ListenerList aList)
{
  int hashCode = ((int) aList->source) ^ ((int) aList->eventType);
  return hashCode;
}

static int compHashEntrys(ListenerList list1, ListenerList list2)
{
  if ((list1->source == list2->source) &&
      (list1->eventType == list2->eventType))
  {
    return 0;
  }
  else
  {
    return 4711;
  }
  
}

/*
 * Free all the ListenerEntrys in the list
 */
static void freeListenerList(ListenerList aList)
{
  int i;

  for (i = 0; i < LLIST_NO_ELEMS; i++)
  {
    if (aList->listenerEntryList[i] != NULL)
    {
      free(aList->listenerEntryList[i]);
      aList->listenerEntryList[i] = NULL;
    }
  }
  
}


/*
 * Write the listenerList for debug purposes
 */
static void writeListenerList(ListenerList aList)
{
  int i;
  
  printf("ListenerList: source: %x, evtype: %d\n",(int)aList->source,
	 aList->eventType);
  for (i=0; i < LLIST_NO_ELEMS; i++)
  {
    if (aList->listenerEntryList[i] != NULL)
    {
      printf("  Entry: hfunc: %x, hdata: %x, newthr: %d\n",
	     (int)aList->listenerEntryList[i]->handlerFunc,
	     (int)aList->listenerEntryList[i]->handlerData,
	     aList->listenerEntryList[i]->makeNewThread);
    }
  }
  printf("--------------------------------\n\n");
}


