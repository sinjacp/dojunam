﻿#include "WorkerManager.h"

using namespace MyBot;

// kyj
void WorkerManager::finishedWithWorker(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	//BWAPI::Broodwar->printf("BuildingManager finished with worker %d", unit->getID());
	if (workerData.getWorkerJob(unit) != WorkerData::Scout)
	{
		workerData.setWorkerJob(unit, WorkerData::Idle, nullptr);
	}
}








////////////////////////////////////////////////////////////////////////////////////////////////
WorkerManager::WorkerManager() 
{
	currentRepairWorker = nullptr;
}

WorkerManager & WorkerManager::Instance() 
{
	static WorkerManager instance;
	return instance;
}

void WorkerManager::update() 
{
	// 1초에 1번만 실행한다
	if (BWAPI::Broodwar->getFrameCount() % 24 != 0) return;

	updateWorkerStatus();
	handleGasWorkers();
	handleIdleWorkers();
	handleMoveWorkers();
	handleCombatWorkers();
	handleRepairWorkers();
}

void WorkerManager::updateWorkerStatus() 
{
	// Drone 은 건설을 위해 isConstructing = true 상태로 건설장소까지 이동한 후, 
	// 잠깐 getBuildType() == none 가 되었다가, isConstructing = true, isMorphing = true 가 된 후, 건설을 시작한다

	// for each of our Workers
	for (auto & worker : workerData.getWorkers())
	{
		//if (workerData.getWorkerJob(worker) == WorkerData::Build && worker->getBuildType() == BWAPI::UnitTypes::None)
		//{
		//	std::cout << "construction worker " << worker->getID() << "buildtype BWAPI::UnitTypes::None " << std::endl;
		//}

		/*
		if (worker->isCarryingMinerals()) {
			std::cout << "mineral worker isCarryingMinerals " << worker->getID() 
				<< " isIdle: " << worker->isIdle()
				<< " isCompleted: " << worker->isCompleted()
				<< " isInterruptible: " << worker->isInterruptible()
				<< " target Name: " << worker->getTarget()->getType().getName()
				<< " job: " << workerData.getWorkerJob(worker)
				<< " exists " << worker->exists()
				<< " isConstructing " << worker->isConstructing()
				<< " isMorphing " << worker->isMorphing()
				<< " isMoving " << worker->isMoving()
				<< " isBeingConstructed " << worker->isBeingConstructed()
				<< " isStuck " << worker->isStuck()
				<< std::endl;
		}
		*/

		if (!worker->isCompleted())
		{
			continue;
		}

		// 게임상에서 worker가 isIdle 상태가 되었으면 (새로 탄생했거나, 그전 임무가 끝난 경우), WorkerData 도 Idle 로 맞춘 후, handleGasWorkers, handleIdleWorkers 등에서 새 임무를 지정한다 
		if ( worker->isIdle() )
		{
			/*
			if ((workerData.getWorkerJob(worker) == WorkerData::Build)
				|| (workerData.getWorkerJob(worker) == WorkerData::Move)
				|| (workerData.getWorkerJob(worker) == WorkerData::Scout)) {

				std::cout << "idle worker " << worker->getID()
					<< " job: " << workerData.getWorkerJob(worker)
					<< " exists " << worker->exists()
					<< " isConstructing " << worker->isConstructing()
					<< " isMorphing " << worker->isMorphing()
					<< " isMoving " << worker->isMoving()
					<< " isBeingConstructed " << worker->isBeingConstructed()
					<< " isStuck " << worker->isStuck()
					<< std::endl;
			}
			*/

			// workerData 에서 Build / Move / Scout 로 임무지정한 경우, worker 는 즉 임무 수행 도중 (임무 완료 전) 에 일시적으로 isIdle 상태가 될 수 있다 
			if ((workerData.getWorkerJob(worker) != WorkerData::Build)
				&& (workerData.getWorkerJob(worker) != WorkerData::Move)
				&& (workerData.getWorkerJob(worker) != WorkerData::Scout))  
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}

		// if its job is gas
		if (workerData.getWorkerJob(worker) == WorkerData::Gas)
		{
			BWAPI::Unit refinery = workerData.getWorkerResource(worker);

			// if the refinery doesn't exist anymore (파괴되었을 경우)
			if (!refinery || !refinery->exists() ||	refinery->getHitPoints() <= 0)
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}

		// if its job is repair
		if (workerData.getWorkerJob(worker) == WorkerData::Repair)
		{
			BWAPI::Unit repairTargetUnit = workerData.getWorkerRepairUnit(worker);
						
			// 대상이 파괴되었거나, 수리가 다 끝난 경우
			if (!repairTargetUnit || !repairTargetUnit->exists() || repairTargetUnit->getHitPoints() <= 0 || repairTargetUnit->getHitPoints() == repairTargetUnit->getType().maxHitPoints())
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}
	}
}


void WorkerManager::handleGasWorkers()
{
	// for each unit we have
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		// refinery 가 건설 completed 되었으면,
		if (unit->getType().isRefinery() && unit->isCompleted() )
		{
			// get the number of workers currently assigned to it
			int numAssigned = workerData.getNumAssignedWorkers(unit);

			// if it's less than we want it to be, fill 'er up
			// 미네랄 일꾼은 적은데 가스 일꾼은 무조건 3~4명인 경우 -> Config::Macro::WorkersPerRefinery 값을 조정해야함
			for (int i = 0; i<(Config::Macro::WorkersPerRefinery - numAssigned); ++i)
			{
				BWAPI::Unit gasWorker = chooseGasWorkerFromMineralWorkers(unit);

				if (gasWorker)
				{
					//std::cout << "set gasWorker " << gasWorker->getID() << std::endl;
					workerData.setWorkerJob(gasWorker, WorkerData::Gas, unit);
				}
			}
		}
	}
}

void WorkerManager::handleIdleWorkers() 
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		// if worker's job is idle 
		if (workerData.getWorkerJob(worker) == WorkerData::Idle || workerData.getWorkerJob(worker) == WorkerData::Default )
		{
			// send it to the nearest mineral patch
			setMineralWorker(worker);
		}
	}
}

void WorkerManager::handleMoveWorkers()
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		// if it is a move worker
		if (workerData.getWorkerJob(worker) == WorkerData::Move)
		{
			WorkerMoveData data = workerData.getWorkerMoveData(worker);

			// 목적지에 도착한 경우 이동 명령을 해제한다
			if (worker->getPosition().getDistance(data.position) < 4) {
				setIdleWorker(worker);
			}
			else {
				CommandUtil::move(worker, data.position);
			}
		}
	}
}



// bad micro for combat workers
void WorkerManager::handleCombatWorkers()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (workerData.getWorkerJob(worker) == WorkerData::Combat)
		{
			BWAPI::Broodwar->drawCircleMap(worker->getPosition().x, worker->getPosition().y, 4, BWAPI::Colors::Yellow, true);
			BWAPI::Unit target = getClosestEnemyUnitFromWorker(worker);

			if (target)
			{
				CommandUtil::attackUnit(worker, target);
			}
		}
	}
}

BWAPI::Unit WorkerManager::getClosestEnemyUnitFromWorker(BWAPI::Unit worker)
{
	if (!worker) return nullptr;

	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 10000;

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		double dist = unit->getDistance(worker);

		if ((dist < 400) && (!closestUnit || (dist < closestDist)))
		{
			closestUnit = unit;
			closestDist = dist;
		}
	}

	return closestUnit;
}

void WorkerManager::stopCombat()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (workerData.getWorkerJob(worker) == WorkerData::Combat)
		{
			setMineralWorker(worker);
		}
	}
}

void WorkerManager::handleRepairWorkers()
{
	if (BWAPI::Broodwar->self()->getRace() != BWAPI::Races::Terran)
	{
		return;
	}

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		// 건물의 경우 아무리 멀어도 무조건 수리. 일꾼 한명이 순서대로 수리
		if (unit->getType().isBuilding() && unit->isCompleted() == true && unit->getHitPoints() < unit->getType().maxHitPoints())
		{
			BWAPI::Unit repairWorker = chooseRepairWorkerClosestTo(unit->getPosition());
			setRepairWorker(repairWorker, unit);
			break;
		}
		// 메카닉 유닛 (SCV, 시즈탱크, 레이쓰 등)의 경우 근처에 SCV가 있는 경우 수리. 일꾼 한명이 순서대로 수리
		else if (unit->getType().isMechanical() && unit->isCompleted() == true && unit->getHitPoints() < unit->getType().maxHitPoints())
		{
			// SCV 는 수리 대상에서 제외. 전투 유닛만 수리하도록 한다
			if (unit->getType() != BWAPI::UnitTypes::Terran_SCV) {
				BWAPI::Unit repairWorker = chooseRepairWorkerClosestTo(unit->getPosition(), 10 * TILE_SIZE);
				setRepairWorker(repairWorker, unit);
				break;
			}
		}

	}
}

BWAPI::Unit WorkerManager::chooseRepairWorkerClosestTo(BWAPI::Position p, int maxRange)
{
	if (!p.isValid()) return nullptr;

    BWAPI::Unit closestWorker = nullptr;
    double closestDist = 100000000;

	if (currentRepairWorker != nullptr && currentRepairWorker->exists() && currentRepairWorker->getHitPoints() > 0)
    {
		return currentRepairWorker;
    }

    // for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker)
		{
			continue;
		}

		if (worker->isCompleted() 
			&& (workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle || workerData.getWorkerJob(worker) == WorkerData::Move))
		{
			double dist = worker->getDistance(p);

			if (!closestWorker || dist < closestDist)
            {
				closestWorker = worker;
                dist = closestDist;
            }
		}
	}

	if (currentRepairWorker == nullptr || currentRepairWorker->exists() == false || currentRepairWorker->getHitPoints() <= 0) {
		currentRepairWorker = closestWorker;
	}

	return closestWorker;
}

BWAPI::Unit WorkerManager::getScoutWorker()
{
    // for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker)
		{
			continue;
		}
		// if it is a scout worker
        if (workerData.getWorkerJob(worker) == WorkerData::Scout) 
		{
			return worker;
		}
	}

    return nullptr;
}

// set a worker to mine minerals
void WorkerManager::setMineralWorker(BWAPI::Unit unit)
{
	if (!unit) return;

	// check if there is a mineral available to send the worker to
	BWAPI::Unit depot = getClosestResourceDepotFromWorker(unit);

	// if there is a valid ResourceDepot (Command Center, Nexus, Hatchery)
	if (depot)
	{
		// update workerData with the new job
		workerData.setWorkerJob(unit, WorkerData::Minerals, depot);
	}
}
BWAPI::Unit WorkerManager::getClosestMineralWorkerTo(BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000;
	
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (!unit)
		{
			continue;
		}

		if (unit->isCompleted()
			&& unit->getHitPoints() > 0
			&& unit->exists()
			&& unit->getType().isWorker()
			&& WorkerManager::Instance().isMineralWorker(unit))
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Unit WorkerManager::getClosestResourceDepotFromWorker(BWAPI::Unit worker)
{
	if (!worker) return nullptr;

	BWAPI::Unit closestDepot = nullptr;
	double closestDistance = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (!unit) continue;
		
		// 가장 가까운, 일꾼수가 꽉 차지않은, 완성된 ResourceDepot (혹은 Lair 나 Hive로 변형중인 건물)을 찾는다
		if (unit->getType().isResourceDepot()
			&& (unit->isCompleted() || unit->getType() == BWAPI::UnitTypes::Zerg_Lair || unit->getType() == BWAPI::UnitTypes::Zerg_Hive) )
		{
			double distance = unit->getDistance(worker);

			// 일단 여러 ResourceDepot 중 하나는 선택되도록 한다
			if (!closestDepot )
			{
				closestDepot = unit;
				closestDistance = distance;
			}
			// 더 가까운 ResourceDepot 이 있고, 일꾼 수가 꽉 차지 않았다면 거기 가도록 한다
			else if (distance < closestDistance
				&& workerData.depotHasEnoughMineralWorkers(unit) == false) 
			{
				closestDepot = unit;
				closestDistance = distance;
			}
		}
	}

	return closestDepot;
}


// other managers that need workers call this when they're done with a unit
void WorkerManager::setIdleWorker(BWAPI::Unit unit)
{
	if (!unit) return;

	workerData.setWorkerJob(unit, WorkerData::Idle, nullptr);
}

// 해당 refinery 로부터 가장 가까운, Mineral 캐고있던 일꾼을 리턴한다
BWAPI::Unit WorkerManager::chooseGasWorkerFromMineralWorkers(BWAPI::Unit refinery)
{
	if (!refinery) return nullptr;

	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;
		
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Minerals))
		{
			double distance = unit->getDistance(refinery);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	return closestWorker;
}

void WorkerManager::setConstructionWorker(BWAPI::Unit worker, BWAPI::UnitType buildingType)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Build, buildingType);
}

BWAPI::Unit WorkerManager::chooseConstuctionWorkerClosestTo(BWAPI::UnitType buildingType, BWAPI::TilePosition buildingPosition, bool setJobAsConstructionWorker, int avoidWorkerID)
{
	// variables to hold the closest worker of each type to the building
	BWAPI::Unit closestMovingWorker = nullptr;
	BWAPI::Unit closestMiningWorker = nullptr;
	double closestMovingWorkerDistance = 0;
	double closestMiningWorkerDistance = 0;

	// look through each worker that had moved there first
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;

		// worker 가 2개 이상이면, avoidWorkerID 는 피한다
		if (workerData.getWorkers().size() >= 2 && avoidWorkerID != 0 && unit->getID() == avoidWorkerID) continue;

		// Move / Idle Worker
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Move || workerData.getWorkerJob(unit) == WorkerData::Idle))
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(BWAPI::Position(buildingPosition));
			if (!closestMovingWorker || distance < closestMovingWorkerDistance)
			{
				if (BWTA::isConnected(unit->getTilePosition(), buildingPosition)) {
					closestMovingWorker = unit;
					closestMovingWorkerDistance = distance;
				}
			}
		}

		// Move / Idle Worker 가 없을때, 다른 Worker 중에서 차출한다 
		if (unit->isCompleted() && workerData.getWorkerJob(unit) != WorkerData::Move && workerData.getWorkerJob(unit) != WorkerData::Idle && workerData.getWorkerJob(unit) != WorkerData::Build)
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(BWAPI::Position(buildingPosition));
			if (!closestMiningWorker || distance < closestMiningWorkerDistance)
			{
				if (BWTA::isConnected(unit->getTilePosition(), buildingPosition)) {
					closestMiningWorker = unit;
					closestMiningWorkerDistance = distance;
				}
			}
		}
	}
	
	/*
	if (closestMiningWorker)
		std::cout << "closestMiningWorker " << closestMiningWorker->getID() << std::endl;
	if (closestMovingWorker)
		std::cout << "closestMovingWorker " << closestMovingWorker->getID() << std::endl;
	*/
	
	BWAPI::Unit chosenWorker = closestMovingWorker ? closestMovingWorker : closestMiningWorker;

	// if the worker exists (one may not have been found in rare cases)
	if (chosenWorker && setJobAsConstructionWorker)
	{
		workerData.setWorkerJob(chosenWorker, WorkerData::Build, buildingType);
	}

	return chosenWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(BWAPI::Unit worker)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Scout, nullptr);
}

// get a worker which will move to a current location
BWAPI::Unit WorkerManager::chooseMoveWorkerClosestTo(BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;

		// only consider it if it's a mineral worker
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Minerals || workerData.getWorkerJob(unit) == WorkerData::Idle))
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(p);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	// return the worker
	return closestWorker;
}

// sets a worker to move to a given location
void WorkerManager::setMoveWorker(BWAPI::Unit worker, int mineralsNeeded, int gasNeeded, BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	double closestDistance = 0;

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;
		
		// only consider it if it's a mineral worker or idle worker
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Minerals || workerData.getWorkerJob(unit) == WorkerData::Idle))
		{
			// if it is a new closest distance, set the pointer
			double distance = unit->getDistance(p);
			if (!closestWorker || distance < closestDistance)
			{
				closestWorker = unit;
				closestDistance = distance;
			}
		}
	}

	if (closestWorker)
	{
		workerData.setWorkerJob(closestWorker, WorkerData::Move, WorkerMoveData(mineralsNeeded, gasNeeded, p));
	}
	else
	{
		//BWAPI::Broodwar->printf("Error, no worker found");
	}
}

void WorkerManager::setCombatWorker(BWAPI::Unit worker)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Combat, nullptr);
}

void WorkerManager::setRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair)
{
	workerData.setWorkerJob(worker, WorkerData::Repair, unitToRepair);
}

void WorkerManager::stopRepairing(BWAPI::Unit worker)
{
	workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}


void WorkerManager::onUnitMorph(BWAPI::Unit unit)
{
	if (!unit) return;

	// if something morphs into a worker, add it
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	{
		workerData.addWorker(unit);
	}

	// if something morphs into a building, it was a worker (Zerg Drone)
	if (unit->getType().isBuilding() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getPlayer()->getRace() == BWAPI::Races::Zerg)
	{
		// 해당 worker 를 workerData 에서 삭제한다
		workerData.workerDestroyed(unit);
	}
}

void WorkerManager::onUnitShow(BWAPI::Unit unit)
{
	if (!unit) return;

	// add the depot if it exists
	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.addDepot(unit);
	}

	// add the worker
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	{
		workerData.addWorker(unit);
	}

	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		rebalanceWorkers();
	}

}

// 일하고있는 resource depot 에 충분한 수의 mineral worker 들이 지정되어 있다면, idle 상태로 만든다
// idle worker 에게 mineral job 을 부여할 때, mineral worker 가 부족한 resource depot 으로 이동하게 된다  
void WorkerManager::rebalanceWorkers()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!workerData.getWorkerJob(worker) == WorkerData::Minerals)
		{
			continue;
		}

		BWAPI::Unit depot = workerData.getWorkerDepot(worker);

		if (depot && workerData.depotHasEnoughMineralWorkers(depot))
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
		else if (!depot)
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
	}
}

void WorkerManager::onUnitDestroy(BWAPI::Unit unit) 
{
	if (!unit) return;

	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.removeDepot(unit);
	}

	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self()) 
	{
		workerData.workerDestroyed(unit);
	}

	if (unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field)
	{
		rebalanceWorkers();
	}
}

bool WorkerManager::isMineralWorker(BWAPI::Unit worker)
{
	if (!worker) return false;

	return workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle;
}

bool WorkerManager::isScoutWorker(BWAPI::Unit worker)
{
	if (!worker) return false;

	return (workerData.getWorkerJob(worker) == WorkerData::Scout);
}

bool WorkerManager::isConstructionWorker(BWAPI::Unit worker)
{
	if (!worker) return false;

	return (workerData.getWorkerJob(worker) == WorkerData::Build);
}

int WorkerManager::getNumMineralWorkers() 
{
	return workerData.getNumMineralWorkers();	
}

int WorkerManager::getNumIdleWorkers() 
{
	return workerData.getNumIdleWorkers();	
}

int WorkerManager::getNumGasWorkers() 
{
	return workerData.getNumGasWorkers();
}

WorkerData  WorkerManager::getWorkerData()
{
	return workerData;
}