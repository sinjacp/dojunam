//kyj.h
#pragma once
#include "Common.h"
#include <exception>
class Kyj{
public:
	static Kyj & Instance();
	void onSendText(std::string text);
};