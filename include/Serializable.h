/*
 * File:	serializable.h
 * Author:	James Letendre
 *
 * Serializable interface
 */
#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "NetworkPackets.h"

class Serializable
{
	virtual Packet* serialize() const = 0;
	virtual bool unserialize( Packet &packet ) = 0;
};

#endif
