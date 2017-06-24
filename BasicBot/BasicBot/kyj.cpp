//kyj.cpp
#include "kyj.h"
Kyj & Kyj::Instance()
{
	static Kyj instance;
	return instance;
}
void Kyj::onSendText(std::string text){
	if (text.find("speed ") == 0){
		try{
			int tmpSpeed = std::stoi(text.substr(text.find(" ") + 1));
			BWAPI::Broodwar->setLocalSpeed(tmpSpeed);
			std::cout << "setLocalSpeed=" << tmpSpeed << std::endl;
		}
		catch (std::exception &e){
			std::cout << e.what() << std::endl;
		}
	}
	//잘안먹음
	else if (text.find("setFrameSkip ") == 0){
		try{
			int tmpSkip = std::stoi(text.substr(text.find(" ") + 1));
			BWAPI::Broodwar->setFrameSkip(tmpSkip);
			std::cout << "setFrameSkip=" << tmpSkip << std::endl;
		}
		catch (std::exception &e){
			std::cout << e.what() << std::endl;
		}
	}
}