#pragma once

class AnimationLoader;

#include <iostream>
//#include "DrawableObject.h"
#include "List.h"
#include "ListWithoutUID.h"
//#include "GameManager.h"
#include "AnimatedObjectType.h"
#include "RegistratedString.h"
#include <SFML/Graphics.hpp>

#define AL_CLASS_MULTIPLIER 10000

using namespace sf;

class AnimationLoader
{
	char xmlfilename[256]; // default: "animations.data"

	uint names_id_counter;
	uint classnames_id_counter;
	uint types_id_counter;
	uint subtypes_id_counter;
	
	List<AnimatedObjectType> aotypes;
	List<RegistratedString> classnames;
	List<RegistratedString> names;

	List<RegistratedString> animtypes;
	List<RegistratedString> animsubtypes;

	ListWithoutUID<Texture> textures;

	AnimationLoader();
public:
		
	AnimationLoader(char * xmlfile);
	~AnimationLoader();

	uint addType(AnimatedObjectType *at);
	void loadTextures();
	AnimatedObjectType * getAOType(char * classname, char * name);
	AnimatedObjectType * getAOType(uint uid);
	uint getAnimationUID(char * type, char * subtype);
};

