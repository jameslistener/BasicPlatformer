#include "GameObject.h"



GameObject::GameObject()
{

}

GameObject::GameObject(Vector2f _coords, bool _is_active)
{
	coords = _coords;
	uid = Mgr.getNewUID();
	is_active = _is_active;
}

GameObject::GameObject(Vector2f _coords, uint _uid, bool _is_active)
{
	coords = _coords;
	uid = _uid;
	is_active = _is_active;
}

void GameObject::Coords(Vector2f c)
{
	coords = c;
}

Vector2f GameObject::Coords()
{
	return coords;
}

uint GameObject::UID()
{
	return uid;
}

bool GameObject::isActive()
{
	return is_active;
}

void GameObject::activate()
{
	is_active = true;
}

void GameObject::disActivate()
{
	is_active = false;
}


GameObject::~GameObject()
{
}
