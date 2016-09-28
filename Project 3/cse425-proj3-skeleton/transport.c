/*
 * transport.c 
 *
 *	Project 3		
 *
 * This file implements the STCP layer that sits between the
 * mysocket and network layers. You are required to fill in the STCP
 * functionality in this file. 
 *
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include "mysock.h"
#include "stcp_api.h"
#include "transport.h"


enum { 	CSTATE_ESTABLISHED = 1,
		LISTEN = 2,
		SYN_SENT = 3,
		SYN_RECD = 4  
	 };    /* you should have more states */  /* I have it already */


/* this structure is global to a mysocket descriptor */
typedef struct
{
	bool_t done;    /* TRUE once connection is closed */

	int connection_state;   /* state of the connection (established, etc.) */
	tcp_seq initial_sequence_num;

	/* any other connection-wide global variables go here */
} context_t;


static void generate_initial_seq_num(context_t *ctx);
static void control_loop(mysocket_t sd, context_t *ctx);


/* initialise the transport layer, and start the main loop, handling
 * any data from the peer or the application.  this function should not
 * return until the connection is closed.
 */
void transport_init(mysocket_t sd, bool_t is_active)
{
	context_t *ctx;

	ctx = (context_t *) calloc(1, sizeof(context_t));
	assert(ctx);

	generate_initial_seq_num(ctx);

	STCPHeader *synPacket, *synAckPacket, *ackPacket, *inPacket = (STCPHeader *) calloc(1,sizeof(STCPHeader));
	// STCPHeader *synAckPacket = (STCPHeader *) calloc(1,sizeof(STCPHeader));
	// STCPHeader *ackPacket = (STCPHeader *) calloc(1,sizeof(STCPHeader));
	
	if(is_active)
	{

		synPacket->th_seq = ctx->initial_sequence_num;
		synPacket->th_off = 5;
		synPacket->th_flags = TH_SYN;
		stcp_network_send(sd,synPacket,sizeof(synPacket),NULL);

		ctx->connection_state = SYN_SENT;

		stcp_network_recv(sd,inPacket,sizeof(inPacket));

		// if(synAckPacket->th_ack == ctx->initial_sequence_num + 1)
		if(inPacket->th_flags & TH_SYN)
		{
			ctx->connection_state = SYN_RECD;

			synAckPacket->th_ack = inPacket->th_seq + 1;
			synAckPacket->th_seq = ctx->initial_sequence_num;
			synAckPacket->th_off = 5;
			synAckPacket->th_flags = (TH_SYN | TH_ACK);

			stcp_network_send(sd,synAckPacket,sizeof(synAckPacket),NULL);

			stcp_network_recv(sd,inPacket,sizeof(inPacket));
		}
		
		if((inPacket->th_flags & (TH_SYN | TH_ACK))) 
		{
			ackPacket->th_seq = ctx->initial_sequence_num + 1;
			ackPacket->th_ack = inPacket->th_seq + 1;
			ackPacket->th_off = 5;
			ackPacket->th_flags = TH_ACK;

			stcp_network_send(sd,ackPacket,sizeof(ackPacket),NULL);
		}
		else
		{
			//error handling
		}
	}

	else
	{
		ctx->connection_state = LISTEN;

		stcp_network_recv(sd,synPacket,sizeof(synPacket));
		if(synPacket->th_flags != TH_SYN)
		{
			//error handling
		}
		else
		{
			synAckPacket->th_ack = synPacket->th_seq + 1;
			synAckPacket->th_seq = ctx->initial_sequence_num;
			synAckPacket->th_off = 5;
			synAckPacket->th_flags = (TH_SYN | TH_ACK);

			stcp_network_send(sd,synAckPacket,sizeof(synAckPacket),NULL);

			ctx->connection_state = SYN_RECD;

			stcp_network_recv(sd,ackPacket,sizeof(ackPacket));
			if(ackPacket->th_flags != TH_ACK || ackPacket->th_ack != ctx->initial_sequence_num + 1)
			{
				//error handling
			}
		}
	}
	/* XXX: you should send a SYN packet here if is_active, or wait for one
	 * to arrive if !is_active.  after the handshake completes, unblock the
	 * application with stcp_unblock_application(sd).  you may also use
	 * this to communicate an error condition back to the application, e.g.
	 * if connection fails; to do so, just set errno appropriately (e.g. to
	 * ECONNREFUSED, etc.) before calling the function.
	 */
	ctx->connection_state = CSTATE_ESTABLISHED;
	stcp_unblock_application(sd);

	control_loop(sd, ctx);

	/* do any cleanup here */
	free(ctx);
}


/* generate random initial sequence number for an STCP connection */
static void generate_initial_seq_num(context_t *ctx)
{
	assert(ctx);

#ifdef FIXED_INITNUM
	/* please don't change this! */
	ctx->initial_sequence_num = 1;
#else
	/* you have to fill this up */
	/*ctx->initial_sequence_num =;*/
#endif
}


/* control_loop() is the main STCP loop; it repeatedly waits for one of the
 * following to happen:
 *   - incoming data from the peer
 *   - new data from the application (via mywrite())
 *   - the socket to be closed (via myclose())
 *   - a timeout
 */
static void control_loop(mysocket_t sd, context_t *ctx)
{
	assert(ctx);
	assert(!ctx->done);

	while (!ctx->done)
	{
		unsigned int event;

		/* see stcp_api.h or stcp_api.c for details of this function */
		/* XXX: you will need to change some of these arguments! */
		event = stcp_wait_for_event(sd, 0, NULL);

		/* check whether it was the network, app, or a close request */
		if (event & APP_DATA)
		{
			/* the application has requested that data be sent */
			/* see stcp_app_recv() */
		}

		/* etc. */
	}
}


/**********************************************************************/
/* our_dprintf
 *
 * Send a formatted message to stdout.
 * 
 * format               A printf-style format string.
 *
 * This function is equivalent to a printf, but may be
 * changed to log errors to a file if desired.
 *
 * Calls to this function are generated by the dprintf amd
 * dperror macros in transport.h
 */
void our_dprintf(const char *format,...)
{
	va_list argptr;
	char buffer[1024];

	assert(format);
	va_start(argptr, format);
	vsnprintf(buffer, sizeof(buffer), format, argptr);
	va_end(argptr);
	fputs(buffer, stdout);
	fflush(stdout);
}



