/*
 * Copyright (c) 2007, 2013 Daniel Corbe
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors, copyright holders or the contributors
 *    may be used to endorese or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS OR CONTRIBUTORS BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, COPYRIGHT ENFRINGEMENT, LOSS
 * OF PROFITS, REVENUE, OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "timer.h"

#ifdef DEBUG
#include <sys/select.h>

int main(int argc, char **argv)
{
	struct timer *timers;
	struct timer *timer;
	struct timeval tv;
	struct timeval set;
	struct timeval next;

	set.tv_sec = 3;
	set.tv_usec = 0;

	gettimeofday(&tv, NULL);

	timers = timer_init();

	/* Set a 3 second timer */
	timer = timer_set(timers, NULL, &set);

	/* Loop forever and reset the timer every time it firest */
	for ( ; ; )
	{
		get_next_offset(timers, NULL, &next);
		select(0, NULL, NULL, NULL, &next);
		printf("TIMER FIRED\n");
		timer = get_expired_timer(timers, NULL);
		timer_reset(timer, NULL);
	}
}
#endif /* DEBUG */

struct timer *timer_init(void)
{
	struct timer *t;

	t = (struct timer *)malloc(sizeof(struct timer));
	t->next = NULL;
	t->prev = NULL;
	t->interval.tv_sec = 0;
	t->interval.tv_usec = 0;

	return(t);
}

struct timer *timer_set(struct timer *timers, struct timeval *ctime, 
			struct timeval *interval)
{
	struct timeval time;
	struct timer *tptr;

	if (ctime == NULL)
	{
		gettimeofday(&time, NULL);
		ctime = &time;
	}

	/* Look for an empty (undused) timer */
	tptr = timers;
	while (tptr->next)
	{
		if (tptr->interval.tv_sec == 0 &&
		    tptr->interval.tv_usec == 0)
			break;

		tptr = tptr->next;
	}

	/* Fails if we've reached the end of the list without finding a usable
	   timer */
	if (tptr->interval.tv_sec != 0 &&
	    tptr->interval.tv_usec != 0)
	{
		tptr->next = (struct timer *)malloc(sizeof(struct timer));
		tptr->next->prev = tptr;
		tptr->next->next = NULL;
		tptr = tptr->next;
	}

	/* Set the timer */
	tptr->interval.tv_sec = interval->tv_sec;
	tptr->interval.tv_usec = interval->tv_usec;

	/* Calculate the expirtation */
	tptr->expires.tv_sec = interval->tv_sec + ctime->tv_sec;
	tptr->expires.tv_usec = interval->tv_usec + interval->tv_usec;
	if (tptr->expires.tv_usec > 999999)
	{
		tptr->expires.tv_sec++;
		tptr->expires.tv_usec = 1000000 - tptr->expires.tv_usec;
	}

	/* Done */
	return(tptr);
}

struct timer *get_next_offset(struct timer *timers, struct timeval *ctime, 
			      struct timeval *offset)
{
	struct timeval time;
	struct timer *tptr;
	struct timer *floor;

	if (ctime == NULL)
	{
		gettimeofday(&time, NULL);
		ctime = &time;
	}

	/* This will search the timer list for the first expiring timer */
	floor = timers;
	for (tptr = timers; tptr; tptr = tptr->next)
	{
		if (tptr->interval.tv_sec > 0 && 
		    tptr->interval.tv_usec > 0)
		{
			if (tptr->expires.tv_sec < floor->expires.tv_sec)
				floor = tptr;
			else if (tptr->expires.tv_sec == floor->expires.tv_sec &&
				 tptr->expires.tv_usec < floor->expires.tv_usec)
				floor = tptr;
			else
				continue;
		}
	}

	/* We need to do one final check here to see if we're looking at
	   an invalid timer */
	if (floor->interval.tv_sec == 0 &&
	    floor->interval.tv_usec == 0)
	{
		offset->tv_sec = 0;
		offset->tv_usec = 0;
		return(NULL);
	}

	/* Calculate the difference between ctime and the scheduled expiry */
	offset->tv_sec = floor->expires.tv_sec - ctime->tv_sec;
	if (floor->expires.tv_usec > ctime->tv_usec)
	{
		offset->tv_sec--;
		offset->tv_usec = floor->expires.tv_usec - 1000000;
	}
	else
	{
		offset->tv_usec = floor->expires.tv_usec;
	}
	
	/* Done */
	return(floor);
}

/* Rearms a timer */
struct timer *timer_reset(struct timer *timer, struct timeval *ctime)
{
	struct timeval time;

	if (ctime == NULL)
	{
		gettimeofday(&time, NULL);
		ctime = &time;
	}

	/* Calculate the expirtation */
	timer->expires.tv_sec = timer->interval.tv_sec + ctime->tv_sec;
	timer->expires.tv_usec = (timer->interval.tv_usec + 
				  timer->interval.tv_usec);
	if (timer->expires.tv_usec > 999999)
	{
		timer->expires.tv_sec++;
		timer->expires.tv_usec = 1000000 - timer->expires.tv_usec;
	}

	/* Done */
	return(timer);	
}

struct timer *timer_destroy(struct timer *timer)
{
	timer->interval.tv_sec = 0;
	timer->interval.tv_usec = 0;
	timer->expires.tv_sec = 0;
	timer->expires.tv_usec = 0;

	return(NULL);
}

/* Return the first expired timer in the list */
struct timer *get_expired_timer(struct timer *timers, struct timeval *ctime)
{
	struct timeval time;
	struct timer *tptr;

	if (ctime == NULL)
	{
		gettimeofday(&time, NULL);
		ctime = &time;
	}

	for (tptr = timers; tptr; tptr = tptr->next)
	{
		if (tptr->expires.tv_sec < ctime->tv_sec)
			return(tptr);
		if (tptr->expires.tv_sec == ctime->tv_sec &&
		    tptr->expires.tv_usec <= ctime->tv_usec)
			return(tptr);
	}

	return(NULL);
}
