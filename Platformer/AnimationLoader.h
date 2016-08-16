#pragma once

#include <iostream>
#include "DrawableObject.h"
#include "List.h"
#include "GameManager.h"
#include "AnimatedType.h"

class AnimationLoader
{
	char xmlfilename[256]; // default: "animations.data"
	
	List<AnimatedType> types;



public:
	AnimationLoader();
	~AnimationLoader();
};

