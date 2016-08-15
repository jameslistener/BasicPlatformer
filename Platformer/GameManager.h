#pragma once
class GameManager;
class Msg;

#include <SFML/Graphics.hpp>
#include "GameObject.h"
#include "List.h"

#define NULL 0

class Msg {
public:
	uint type;
	union {
		struct {
			int x;
		};
	};
	
	uint UID() { return 0; };
};

class GameManager
{
private:
	uint idCounter;
	List<GameObject> objs;
	List<Msg> msgs;

	void SendToAll(Msg *m);

public:
	GameManager();
	~GameManager();

	uint getNewUID();
	void addNewObject(GameObject * go);

	void Update(uint time_elapsed);
	void SendMsg(Msg *m);
	void ReadMsgs();
	void Draw();

};

extern GameManager Mgr;
extern sf::RenderWindow window;
