/**
 * @file TrafficLight.cpp
 * @author Stephen Welch, Susanna Maria
 * @version 0.1
 * @date 2020-07-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_messages.back());
    _messages.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>

void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _messages.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        const TrafficLightPhase phase = _queue.receive();
        if (phase == green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
// and toggles the current phase of the traffic light between red and green and sends an update method
// to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
// Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
void TrafficLight::cycleThroughPhases()
{
    const int min = 4000;
    const int max = 6000;

    // How to use uniform distribution https://stackoverflow.com/a/19728404

    std::random_device rd;                            // only used once to initialise (seed) engine
    std::mt19937 rng(rd());                           // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased

    auto phase_duration = uni(rng);

    std::chrono::high_resolution_clock::time_point phase_start = std::chrono::high_resolution_clock::now();

    while (true)
    {
        // wait for certain amount of time

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const std::chrono::high_resolution_clock::time_point phase_end = std::chrono::high_resolution_clock::now();
        auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count();

        if (time_elapsed >= phase_duration)
        {
            if (_currentPhase == TrafficLightPhase::red)
            {
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _currentPhase = TrafficLightPhase::red;
            }
            _queue.send(std::move(_currentPhase));
            phase_start = std::chrono::high_resolution_clock::now();
            phase_duration = uni(rng);
        }
    }
}
