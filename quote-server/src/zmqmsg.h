//Copyright (C) Apr 2012 Jorge Silva, Gordon Tani
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA./*
//
// This is part of a Multicast Quote Feed Server for :
// Toronto TL1 and Montreak HSVF depth quotes
// This will take snapshots of current quotes in memory

#ifndef ZMQMSG_H_
#define ZMQMSG_H_
#include "zmq.h"
#include "czmq.h"
#include "QuoteLib.pb-c.h"

//Message broken down into 3 frames
//  Message is formatted on wire as 3 frames:
//  frame 0: key (0MQ string)
//  frame 2: body (blob)
#define FRAME_KEY       0
#define FRAME_BODY      1
#define KVMSG_FRAMES    2

typedef struct _kvmsg {
	//  Presence indicators for each frame
	int present[KVMSG_FRAMES];
	//  Corresponding 0MQ message frames, if any
	zmq_msg_t frame[KVMSG_FRAMES];
	//  Key, copied into safe C string
	char key[QUOTE_LIB__CONSTANTS__KVMSG_KEY_MAX + 1];
	// generic message
	void * value;
	//type of Message
	int msg_type;
	// size of Message
	size_t props_size;
} kvmsg_t;

//  Constructor, Allocates base structure
kvmsg_t * kvmsg_new();
// initialse the kvmsg
void
kvmsg_init(kvmsg_t *self, char * key_name, int msg_type);
// initisalise the value
kvmsg_t * kvmsg_init_value(kvmsg_t *self);
// Zero values
void kvmsn_zero(kvmsg_t *);
//  Destructor
void
kvmsg_destroy(kvmsg_t **self_p);

void kvmsg_dispatch(kvmsg_t *self, void *publicskt, void *inprocskt);

//  Reads key-value message from socket, returns new kvmsg instance.
kvmsg_t *
kvmsg_recv(void *socket);
//  Send key-value message to socket; any empty frames are sent as such.
void
kvmsg_send(kvmsg_t *self, void *socket);
//  Return key from last read message, if any, else NULL
char *
kvmsg_key(kvmsg_t *self);
//  Return body from last read message, if any, else NULL
byte *
kvmsg_body(kvmsg_t *self);
//  Return body size from last read message, if any, else zero
size_t
kvmsg_size(kvmsg_t *self);
//  Set message key as provided
void
kvmsg_set_key(kvmsg_t *self, char *key);

void
kvmsg_dump(kvmsg_t *self);

//  Runs self test of class
int
kvmsg_test(int verbose);

#endif /* ZMQMSG_H_ */
