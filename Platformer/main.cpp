#include <chrono>
#include <iostream>
#include "GameManager.h"
#include "GameObject.h"
#include "DrawableObject.h"
#include "Block.h"
#include "PlayerCharacter.h"

GameManager Mgr;
sf::RenderWindow window;

int main()
{
	//setlocale(LC_ALL, "RUSSIAN");

	window.create(VideoMode(500, 500), L"Block");

	Texture tileset, characterTS;
	
	if (tileset.loadFromFile("images/basic.png"))
		printf("picture loaded");
	else
		printf("picture NOT loaded");
		
	Vector2f coords = Vector2f(0, 0);
	Vector2f size = Vector2f(50, 50);
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			coords.x = j * 50; coords.y = i * 50;
			//Mgr.addNewObject(new Block(coords, size, &tileset, IntRect(25, 15, 50, 50)));
		}
	if (characterTS.loadFromFile("images/jackTS.png"))
		printf("picture loaded");
	else
		printf("picture NOT loaded");
	IntRect * charIdleL = new IntRect(0, 0, 80, 96);
	IntRect * charIdleR = new IntRect(0, 192, 80, 96);
	IntRect * charGoL = new IntRect [4];
	charGoL[0] = IntRect(80, 0, 80, 96);
	charGoL[1] = IntRect(160, 0, 80, 96);
	charGoL[2] = IntRect(240, 0, 80, 96);
	charGoL[3] = IntRect(160, 0, 80, 96);
	IntRect * charGoR = new IntRect[4];
	charGoR[0] = IntRect(80, 192, 80, 96);
	charGoR[1] = IntRect(160, 192, 80, 96);
	charGoR[2] = IntRect(240, 192, 80, 96);
	charGoR[3] = IntRect(160, 192, 80, 96);
	PlayerCharacter * pc = new PlayerCharacter(Vector2f(100,100), Vector2f(80,96), 0, NULL);
	pc->addAnimation(new Animation(pc, A_T_IDLE, A_S_LEFT, 1, 0, &characterTS, charIdleL));
	pc->addAnimation(new Animation(pc, A_T_IDLE, A_S_RIGHT, 1, 0, &characterTS, charIdleR));
	pc->addAnimation(new Animation(pc, A_T_WALK, A_S_LEFT, 4, 100000, &characterTS, charGoL));
	pc->addAnimation(new Animation(pc, A_T_WALK, A_S_RIGHT, 4, 100000, &characterTS, charGoR));
	pc->playAnimation(A_T_WALK, A_S_RIGHT);
	Mgr.addNewObject(pc);

	
	auto start = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	uint micros = 1;
	start = std::chrono::high_resolution_clock::now();

	while (window.isOpen())
	{
		

		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
				window.close();
		}
		elapsed = std::chrono::high_resolution_clock::now() - start;
		micros = (uint) std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		start = std::chrono::high_resolution_clock::now();
		
		Mgr.Update(micros);
		window.clear();
		Mgr.Draw();
		window.display();
	}

	return 0;
}