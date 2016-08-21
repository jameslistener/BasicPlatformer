#pragma once
#include<iostream>
#include"List.h"

typedef unsigned int uint;

class RegistratedString
{
private:
	char str[256];
	uint uid;
	RegistratedString();
public:
	RegistratedString(char * str, uint uid);
	uint UID();
	void getStr(char * str);
	static uint getUIDbyName(List<RegistratedString> * list, char * name);
	static RegistratedString * getRSbyName(List<RegistratedString> * list, char * name);
};