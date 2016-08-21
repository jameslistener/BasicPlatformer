//#include <chrono>
#include <iostream>
#include "GameManager.h"
#include "GameObject.h"
#include "DrawableObject.h"
#include "Block.h"
#include "PlayerCharacter.h"
#include "main.h"
#include <string.h>

GameManager Mgr;
sf::RenderWindow window;

int main()
{
	//setlocale(LC_ALL, "RUSSIAN");

	window.create(VideoMode(500, 500), L"Block");

	Mgr.initAnimationLoader(NULL);
		
	Vector2f coords = Vector2f(0, 0);
	Vector2f size = Vector2f(50, 50);
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			coords.x = j * 50; coords.y = i * 50;
			Mgr.addNewObject(new Block(coords, size));
		}
	
	PlayerCharacter * pc = new PlayerCharacter(Vector2f(100,100), Vector2f(80,96));
	pc->playAnimation("WALK", "LEFT");
	Mgr.addNewObject(pc);
	pc = new PlayerCharacter(Vector2f(100, 300), Vector2f(80, 96));
	pc->playAnimation("WALK", "RIGHT");
	Mgr.addNewObject(pc);
	pc = new PlayerCharacter(Vector2f(300, 300), Vector2f(80, 96));
	pc->playAnimation("WALK", "LEFT");
	Mgr.addNewObject(pc);
	pc = new PlayerCharacter(Vector2f(300, 100), Vector2f(80, 96));
	pc->playAnimation("WALK", "RIGHT");
	Mgr.addNewObject(pc);


	
	/*auto start = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	uint micros = 1;
	start = std::chrono::high_resolution_clock::now();*/
	Clock clock;
	uint micros = clock.getElapsedTime().asMicroseconds();
	clock.restart();
	
	Font font;
	font.loadFromFile("CyrilicOld.ttf");
	Text text("", font, 20);
	text.setColor(Color::Red);//покрасили текст в красный. если убрать эту строку, то по умолчанию он белый
	text.setStyle(Text::Bold);
	text.setPosition(20, 20);
	char fps[10];
	int fps_counter = 0, fps_av = 0, fps_elapsed = 0;;

	while (window.isOpen())
	{
		

		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
				window.close();
		}
		/*elapsed = std::chrono::high_resolution_clock::now() - start;
		micros = (uint) std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		start = std::chrono::high_resolution_clock::now();*/
		micros = clock.getElapsedTime().asMicroseconds();
		clock.restart();
		
		Mgr.Update(micros);
		window.clear();
		Mgr.Draw();
		
		
		fps_av += 1000000 / micros;
		fps_counter++;
		fps_elapsed += micros;
		if (fps_elapsed>=500000)
		{
			_itoa_s(fps_av / fps_counter, fps, 10);
			text.setString(fps);
			fps_av = 0;
			fps_counter = 0;
			fps_elapsed = 0;
		}

		window.draw(text);
		window.display();
	}

	return 0;
}