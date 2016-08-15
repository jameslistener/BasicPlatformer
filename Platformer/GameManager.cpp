#include "GameManager.h"

using namespace sf;

void GameManager::SendToAll(Msg * m)
{
	for (GameObject * curr = objs.startLoopObj(); curr != NULL; curr = objs.nextStepObj())
		curr->SendMsg(m);
}

GameManager::GameManager()
{
	idCounter = 0;
}


GameManager::~GameManager()
{
}

uint GameManager::getNewUID()
{
	return ++idCounter;
}

void GameManager::addNewObject(GameObject * go)
{
	objs.push(go);
}

void GameManager::Update(uint time_elapsed)
{
	for (GameObject * curr = objs.startLoopObj(); curr != NULL; curr = objs.nextStepObj())
		curr->Update(time_elapsed);
}

void GameManager::SendMsg(Msg *m)
{
	msgs.push(m);
}

void GameManager::ReadMsgs()
{
	Msg *m;
	while(m = msgs.popObj())
	{
		switch (m->type)
		{
		case 1:
			break;
		default:
			SendToAll(m);
		}
		delete m;
	}
}

void GameManager::Draw()
{
	for (GameObject * curr = objs.startLoopObj(); curr != NULL; curr = objs.nextStepObj())
		curr->Draw();
}
