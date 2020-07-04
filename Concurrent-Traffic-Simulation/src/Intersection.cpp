/**
 * @file Intersection.cpp
 * @author Stephen Welch, Susanna Maria
 * @brief Intersection and WaitingVehicles class Implementation 
 * @version 0.1
 * @date 2020-07-04
 * 
 * @copyright Copyright (c) 2020 MIT License
 * 
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <random>

#include "Street.h"
#include "Intersection.h"
#include "Vehicle.h"

/**
 * Get Size of waiting vehicles in Intersection
 * 
 * @return int 
 */
int WaitingVehicles::getSize()
{
    std::lock_guard<std::mutex> lock(_mutex);

    return _vehicles.size();
}

/**
 * Store new vehicle and its promise in the vectors
 * 
 * @param vehicle 
 * @param promise 
 */
void WaitingVehicles::pushBack(std::shared_ptr<Vehicle> vehicle, std::promise<void> &&promise)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _vehicles.push_back(vehicle);
    _promises.push_back(std::move(promise));
}

/**
 * Permit the entering into intersection to the first vehicle in the vector
 * 
 */
void WaitingVehicles::permitEntryToFirstInQueue()
{
    std::lock_guard<std::mutex> lock(_mutex);

    // get entries from the front of both queues
    auto firstPromise = _promises.begin();
    auto firstVehicle = _vehicles.begin();

    // fulfill promise and send signal back that permission to enter has been granted
    firstPromise->set_value();

    // remove front elements from both queues
    _vehicles.erase(firstVehicle);
    _promises.erase(firstPromise);
}

/**
 * Construct a new Intersection object
 * 
 */
Intersection::Intersection()
{
    _type = ObjectType::objectIntersection;
    _isBlocked = false;
}

/**
 * Add Street to intersection
 * 
 * @param street 
 */
void Intersection::addStreet(std::shared_ptr<Street> street)
{
    _streets.push_back(street);
}

/**
 * Get possible strrets from intersection, ignore the street you come from
 * 
 * @param incoming 
 * @return std::vector<std::shared_ptr<Street>> 
 */
std::vector<std::shared_ptr<Street>> Intersection::queryStreets(std::shared_ptr<Street> incoming)
{
    // store all outgoing streets in a vector ...
    std::vector<std::shared_ptr<Street>> outgoings;
    for (auto it : _streets)
    {
        if (incoming->getID() != it->getID()) // ... except the street making the inquiry
        {
            outgoings.push_back(it);
        }
    }

    return outgoings;
}


/**
 * adds a new vehicle to the queue and returns once the vehicle is allowed to enter
 * 
 * @param vehicle 
 */
void Intersection::addVehicleToQueue(std::shared_ptr<Vehicle> vehicle)
{
    std::unique_lock<std::mutex> lck(_mtx);
    std::cout << "Intersection #" << _id << "::addVehicleToQueue: thread id = " << std::this_thread::get_id() << std::endl;
    lck.unlock();

    // add new vehicle to the end of the waiting line
    std::promise<void> prmsVehicleAllowedToEnter;
    std::future<void> ftrVehicleAllowedToEnter = prmsVehicleAllowedToEnter.get_future();
    _waitingVehicles.pushBack(vehicle, std::move(prmsVehicleAllowedToEnter));

    // wait until the vehicle is allowed to enter
    ftrVehicleAllowedToEnter.wait();
    lck.lock();
    std::cout << "Intersection #" << _id << ": Vehicle #" << vehicle->getID() << " is granted entry." << std::endl;

    // FP.6b : use the methods TrafficLight::getCurrentPhase and TrafficLight::waitForGreen to block the execution until the traffic light turns green.
    const TrafficLightPhase phase = _trafficLight.getCurrentPhase();
    if (phase == red)
    {
        _trafficLight.waitForGreen();
    }
    lck.unlock();
}

/**
 * Unblock the intersection because car has left
 * 
 * @param vehicle 
 */
void Intersection::vehicleHasLeft(std::shared_ptr<Vehicle> vehicle)
{
    //std::cout << "Intersection #" << _id << ": Vehicle #" << vehicle->getID() << " has left." << std::endl;

    // unblock queue processing
    this->setIsBlocked(false);
}

/**
 * set blocked state of intersection
 * 
 * @param isBlocked 
 */
void Intersection::setIsBlocked(bool isBlocked)
{
    _isBlocked = isBlocked;
    std::cout << "Intersection #" << _id << " isBlocked=" << isBlocked << std::endl;
}


/**
 * virtual function which is executed in a thread
 * 
 */
void Intersection::simulate() // using threads + promises/futures + exceptions
{
    // FP.6a : In Intersection.h, add a private member _trafficLight of type TrafficLight. At this position, start the simulation of _trafficLight.
    _trafficLight.simulate();
    // launch vehicle queue processing in a thread
    threads.emplace_back(std::thread(&Intersection::processVehicleQueue, this));
}

/**
 * process queue of vehicles who wants to enter the intersections
 * 
 */
void Intersection::processVehicleQueue()
{
    // print id of the current thread
    //std::cout << "Intersection #" << _id << "::processVehicleQueue: thread id = " << std::this_thread::get_id() << std::endl;

    // continuously process the vehicle queue
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // only proceed when at least one vehicle is waiting in the queue
        if (_waitingVehicles.getSize() > 0 && !_isBlocked)
        {
            // set intersection to "blocked" to prevent other vehicles from entering
            this->setIsBlocked(true);

            // permit entry to first vehicle in the queue (FIFO)
            _waitingVehicles.permitEntryToFirstInQueue();
        }
    }
}

/**
 * get status of trafficlight
 * 
 * @return true its green
 * @return false its red
 */
bool Intersection::trafficLightIsGreen()
{
    // please include this part once you have solved the final project tasks
    if (_trafficLight.getCurrentPhase() == TrafficLightPhase::green)
        return true;
    else
        return false;
}