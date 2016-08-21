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

void GameManager::initAnimationLoader(char * xmlfilename)
{
	animLoader = new AnimationLoader(xmlfilename);
	animLoader->loadTextures();
}

uint GameManager::getNewUID()
{
	return ++idCounter;
}

void GameManager::addNewObject(GameObject * go)
{
	objs.push(go);
}

AnimationLoader * GameManager::getAnimationLoader()
{
	return animLoader;
}

void GameManager::Update(uint time_elapsed)
{
	//time_elapsed /= 1000;
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
