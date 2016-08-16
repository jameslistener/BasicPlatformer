#pragma once

class AnimatedType;

#include "Animation.h"
#include "List.h"
#include "GameObject.h"
#include <SFML/Graphics.hpp>

using namespace sf;

class AnimatedType
{
private:
	char name[256];
	char classname[256];
	char texturefile[256];
	Texture * texture;
	Vector2i size;
	List<Animation> anims;

	uint uid;

	AnimatedType(); //so no one can create empty object
public:

	AnimatedType(char * name, char * classname, char * texturefile, Vector2i &size);
	~AnimatedType();

	void addAnimation(Animation * a);
	
	// getters
	uint UID();
	char * getName();
	char * getClassName();
	Texture * getTexture();
	Vector2i getSize();
	
	int copyAnimations(Animation ** animations);
	

};

