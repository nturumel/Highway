#include "BehaviourPlanner.h"



void BehaviourPlanner::nearestCar()
{
	
	// initialise maxSpeed vector
	_maxLaneSpeeds = vector<double>(nlane, maxVel);
	for (int i = 0; i < nlane; ++i)
	{
		_relCars.push_back({ nullptr,nullptr });
	}

	// fill up rel vector
	for (int i = 0; i < nlane; ++i)
	{
		car* closestBack = nullptr;
		car* closestFront = nullptr;

		double backDistance = maxReturn;
		double frontDistance = maxReturn;

		for (car& othercar : _hashCar[i])
		{
			if (othercar._distance >= 0) // other car in front
			{
				if (othercar._distance < frontDistance)
				{
					frontDistance = othercar._distance;
					closestFront = &othercar;
				}
			}
			else
			{
				if (-othercar._distance < backDistance)
				{
					backDistance = -othercar._distance;
					closestBack = &othercar;
				}

			}

		}

		_relCars[i] = vector<car*>{ closestBack ,closestFront };

		if (closestBack && closestFront && ((closestBack->_distance < 1.5 * redZone)))
		{
			_maxLaneSpeeds[i] = (closestBack->_speed + closestFront->_speed);
			_maxLaneSpeeds[i] /= 2;
		}
		else if (closestFront && (closestFront->_distance < 1.5 * redZone))
		{
			_maxLaneSpeeds[i] = closestFront->_speed;
		}
		else
		{
			_maxLaneSpeeds[i] = maxVel;
		}

	}

	/*

	// debug info
	cout << "hashcars" << endl;
	for (int i = 0; i < nlane; ++i)
	{
		cout << "i: " << i << endl;
		for (int j = 0; j < _hashCar[i].size(); ++j)
		{
			cout << (_hashCar[i][j])._speed << ":" << (_hashCar[i][j])._distance << ":" << (_hashCar[i][j])._s << ",";
		}
		cout << endl;
	}
	cout << "relcars" << endl;
	for (int i = 0; i < nlane; ++i)
	{
		cout << "i: " << i << endl;
		for (int j = 0; j < _relCars[i].size(); ++j)
		{
			if (_relCars[i][j])
				cout << _relCars[i][j]->_speed << ",";
		}
		cout << endl;
	}
	cout << "speeds" << endl;
	for (int i = 0; i < _maxLaneSpeeds.size(); ++i)
	{
		cout << "i: " << i << ":" << _maxLaneSpeeds[i] << endl;
	}
	cout << "Sensed data end" << endl;
	*/

}

// cost functions
double BehaviourPlanner::laneChangeCost(int lane)
{
	return (fabs(_carCurr._lane - lane));
}
double BehaviourPlanner::speedChangeCost(int lane)
{
	car* front = _relCars[lane][1];
	if (!front)
	{
		return 0;
	}

	double deltaVel = fabs(_carCurr._speed - _maxLaneSpeeds[lane]);

	return deltaVel;
}
double BehaviourPlanner::speedCost(int lane)
{
	return fabs(_maxLaneSpeeds[lane] - maxVel);
}
double BehaviourPlanner::safetyCost(int lane)
{
	// if same lane the return 0
	if (_carCurr._lane == lane)
	{
		return 0;
	}

	// we only consider the cars in front

	// cost of current lane, if there is a car within red zone return max
	car* front = _relCars[_carCurr._lane][1];
	if (front && front->_distance < redZone / 2)
	{
		return maxReturn;
	}

	front = _relCars[lane][1];
	if (front)
	{
		if (fabs(front->_s) < redZone)
		{
			return maxReturn;
		}
		else
		{
			return (1 / front->_distance);
		}

	}

	return 0;

}
double BehaviourPlanner::bufferCost(int lane)
{

	// Initial lane, see if any in 30
	car* front = (_relCars[_carCurr._lane][1]);
	car* back = (_relCars[_carCurr._lane][0]);

	double cost = 0;

	if (front || back)
	{
		double initialCost = 1;
		if (front)
		{
			initialCost *= (1 / fabs(front->_s));
		}
		if (back)
		{
			initialCost *= (1 / fabs(back->_s));
		}
		if (initialCost > 1)
		{
			cost = initialCost;
		}
	}

	front = (_relCars[lane][1]);
	back = (_relCars[lane][0]);
	if (front || back)
	{
		double finalCost = 1;
		if (front)
		{
			finalCost *= (1 / fabs(front->_s));
		}
		if (back)
		{
			finalCost *= (1 / fabs(back->_s));
		}
		if (finalCost > 1)
		{
			if (cost)
			{
				cost *= finalCost;
			}
			else
			{
				cost = finalCost;
			}

		}
	}

	return cost;
}

bool BehaviourPlanner::collision()
{
	car* front = _relCars[_carCurr._lane][1];
	if (front)
	{
		return ((_relCars[_carCurr._lane][1])->_distance < redZone);

	}
	return false;
}

BehaviourPlanner::BehaviourPlanner(car& carCurr, int prevSize, vector<vector<double>>& sensor_fusion) :_carCurr(carCurr), _prevSize(prevSize)
{
	// load the weights 
	streamIn();


	cout << "\t" << "In Constuctor" << endl;
	//set prevSize
	_prevSize = std::min(maxUsePrev, prevSize);

	cout << "\t" << "Previous size: " << _prevSize << endl;

	// initialise hashCar
	for (auto sensed : sensor_fusion)
	{
		double x = sensed[1];
		double y = sensed[2];
		double vx = sensed[3];
		double vy = sensed[4];
		double s = sensed[5];
		double d = sensed[6];

		double speed = sqrt(vx * vx + vy * vy);
		double yaw = atan2(vy, vx);
		int lane = (d / laneWidth);

		if (0 <= lane && lane < nlane)
		{
			car temp(x, y, s, d, yaw, lane, speed);
			if (lane == _carCurr._lane && _carCurr._s > s)
			{
				cout << "Right Behind" << endl;
				temp._distance = (s)-_carCurr._s;

			}
			else
			{
				temp._distance = (s + _prevSize * 0.02 * speed) - _carCurr._s;
			}
			_hashCar[lane].emplace_back(temp);
		}
	}

	// populate the lane vector
	nearestCar();

	cout << "\t" << "Nearest Car" << endl;

	// collise or not
	_collide = collision();

	cout << "\t" << "Collision: " << _collide << endl;

	// set next action
	if (!_collide)
	{
		_next = make_pair(_maxLaneSpeeds[_carCurr._lane], _carCurr._lane);
	}
	else
	{
		_next = choseAction();
	}

	cout << "\t" << "Next action and end: " << _next.first << "," << _next.second << endl;

}

pair<double, int> BehaviourPlanner::choseAction()
{

	double max = maxReturn;
	pair<double, int> result = make_pair(_maxLaneSpeeds[_carCurr._lane], _carCurr._lane);

	if (_carCurr._speed < 10)
	{
		return result;
	}

	for (int i = 0; i < nlane; ++i)
	{
		if (i > _carCurr._lane + 1 || i < _carCurr._lane - 1)
		{
			continue;
		}
		else
		{
			double cost = 0;

			double costAdd;
			costAdd = laneChangeCost(i);
			cout << "Lane change cost: " << costAdd << endl;
			cost += laneChangeFactor * costAdd;

			costAdd = speedChangeCost(i);
			cout << "Speed change cost: " << costAdd << endl;
			cost += speedChangeFactor * costAdd;

			costAdd = speedCost(i);
			cout << "Max speed change cost: " << costAdd << endl;
			cost += speedFactor * costAdd;

			costAdd = bufferCost(i);
			cout << "Buffer cost: " << costAdd << endl;
			cost += bufferFactor * costAdd;


			costAdd = safetyCost(i);
			cout << "Safety cost: " << costAdd << endl;
			cost += safetyFactor * costAdd;

			if (max > cost)
			{
				max = cost;
				result = make_pair(_maxLaneSpeeds[i], i);
			}

		}

	}
	return result;
}

pair<double, int> BehaviourPlanner::returnNextAction()
{
	return _next;
}