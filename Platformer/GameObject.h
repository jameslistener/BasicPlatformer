#pragma once
class GameObject;

#include "GameManager.h"

using namespace sf;
typedef unsigned int uint;

class GameObject
{
protected:
	Vector2f coords;
	uint uid;
	bool is_active;

public:
	GameObject();
	GameObject(Vector2f _coords, bool _is_active = true);
	GameObject(Vector2f _coords, uint _uid, bool _is_active = true);

	void Coords(Vector2f c);
	Vector2f Coords();
	uint UID();
	
	bool isActive();
	void activate();
	void disActivate();

	virtual void Update(uint time_elapsed) = 0;
	virtual void SendMsg(Msg * msg) = 0;
	virtual void Draw() = 0;

	~GameObject();
};

