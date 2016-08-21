#include"RegistratedString.h"

RegistratedString::RegistratedString(char * str, uint uid)
{
	strcpy_s(this->str, str);
	this->uid = uid;
}

uint RegistratedString::UID()
{
	return uid;
}

void RegistratedString::getStr(char * str)
{
	strcpy_s(str, 256, this->str);
}

uint RegistratedString::getUIDbyName(List<RegistratedString>* list, char * name)
{
	char str[256];
	for (RegistratedString * rs = list->startLoopObj(); rs != NULL; rs = list->nextStepObj())
	{
		rs->getStr(str);
		if (strcmp(name, str) == 0)
			return rs->uid;
	}
	return 0;
}

RegistratedString * RegistratedString::getRSbyName(List<RegistratedString>* list, char * name)
{
	char str[256];
	for (RegistratedString * rs = list->startLoopObj(); rs != NULL; rs = list->nextStepObj())
	{
		rs->getStr(str);
		if (strcmp(name, str) == 0)
			return rs;
	}
	return NULL;
}
