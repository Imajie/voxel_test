/*
 * File:	GameObject.h
 *
 * Base game object
 */
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Serializable.h"

class GameObject : public Serializable
{
	public:
		// constructor/destructor
		GameObject() {}
		virtual ~GameObject() {}

		// don't implement serialize here, leave to subclasses
	private:
};

#endif

