#pragma once;

#include "Common.h"
#include "MicroManager.h"

namespace MyBot
{
class MedicManager : public MicroManager
{
public:

	MedicManager();
	void executeMicro(const BWAPI::Unitset & targets);
};
}