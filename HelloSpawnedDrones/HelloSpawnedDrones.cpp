// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "common/common_utils/StrictMode.hpp"
STRICT_MODE_OFF
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "rpc/rpc_error.h"
STRICT_MODE_ON

#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "common/common_utils/FileSystem.hpp"
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>


void runSingleClient(uint16_t port, int ordinal)
{
	using namespace msr::airlib;
	const char host[] = "localhost";
	float timeout_s = 60;

	try
	{
		msr::airlib::MultirotorRpcLibClient *client = new msr::airlib::MultirotorRpcLibClient(host, port, timeout_s);
		std::cout << "Confirming connections..." << std::endl;
		client->confirmConnection();

		std::string vehicleName = "UAV_" + std::to_string(ordinal);
		std::cout << "Vehicle name:" << vehicleName << std::endl;

		client->simAddVehicle(vehicleName, "simpleflight", "", 0, 5.0f * (ordinal + 1), 0);

		// This is a bit crude, but give it a moment to settle on the ground, else takeoff will fail
		std::this_thread::sleep_for(std::chrono::duration<double>(2));

		// moveByVelocityZ is an offboard operation, so we need to set offboard mode.
		client->enableApiControl(true, vehicleName);
		client->armDisarm(true, vehicleName);

		auto groundPosition = client->getMultirotorState(vehicleName).getPosition();
		float groundZ = groundPosition.z(); // current position (NED coordinate system).  

		float takeoffTimeout = 5;
		std::cout << "Initiating takeoff for " << vehicleName << "..." << std::endl;
		client->takeoffAsync(takeoffTimeout, vehicleName)->waitOnLastTask();
		std::cout << "Completed takeoff for " << vehicleName << "..." << std::endl;

		const float speed = 3.0f;

		// switch to explicit hover mode so that this is the fallback when 
		// move* commands are finished.
		std::cout << "Initiating hover for " << vehicleName << "..." << std::endl;
		client->hoverAsync(vehicleName)->waitOnLastTask();
		std::cout << "Completed hover for " << vehicleName << "..." << std::endl;

		auto position = client->getMultirotorState(vehicleName).getPosition();
		float duration = 1;
		float z = position.z(); // current position (NED coordinate system).  

		// Altitude difference between each platform, in meters
		const float altitudeDelta = 1.0f;

		z -= ordinal * altitudeDelta;
		float timeout = 10.0f;
		client->moveToZAsync(z, speed, timeout, msr::airlib::YawMode(), -1.0f, 1.0f, vehicleName)->waitOnLastTask();

		std::cout << "Completed move to z " << z << " for " << vehicleName << "..." << std::endl;
		std::cout << "Flying in a 10m box pattern at 3 m/s velocity" << std::endl;

		const float size = 5.0f;
		duration = size / speed;
		DrivetrainType driveTrain = DrivetrainType::ForwardOnly;
		YawMode yaw_mode(true, 0);

		position = client->getMultirotorState(vehicleName).getPosition();
		std::cout << "Position of " << port << ": " << position << std::endl;
		z = position.z(); // current position (NED coordinate system).  

		std::cout << "moveByVelocityZ(" << speed << ", 0, " << z << "," << duration << ")" << std::endl;
		client->moveByVelocityZAsync(speed, 0, z, duration, driveTrain, yaw_mode, vehicleName);
		std::this_thread::sleep_for(std::chrono::duration<double>(duration));
		std::cout << "moveByVelocityZ(0, " << speed << "," << z << "," << duration << ")" << std::endl;
		client->moveByVelocityZAsync(0, speed, z, duration, driveTrain, yaw_mode, vehicleName);
		std::this_thread::sleep_for(std::chrono::duration<double>(duration));
		std::cout << "moveByVelocityZ(" << -speed << ", 0, " << z << "," << duration << ")" << std::endl;
		client->moveByVelocityZAsync(-speed, 0, z, duration, driveTrain, yaw_mode, vehicleName);
		std::this_thread::sleep_for(std::chrono::duration<double>(duration));
		std::cout << "moveByVelocityZ(0, " << -speed << "," << z << "," << duration << ")" << std::endl;
		client->moveByVelocityZAsync(0, -speed, z, duration, driveTrain, yaw_mode, vehicleName);

		std::this_thread::sleep_for(std::chrono::duration<double>(duration));

		client->moveToZAsync(groundZ - 0.5f, speed, timeout, msr::airlib::YawMode(), -1.0f, 1.0f, vehicleName)->waitOnLastTask();

		std::cout << "Hovering..." << std::endl;
		client->hoverAsync(vehicleName)->waitOnLastTask();

		client->enableApiControl(true, vehicleName);

		std::cout << "Landing..." << std::endl;
		client->landAsync(timeout, vehicleName)->waitOnLastTask();
		std::this_thread::sleep_for(std::chrono::duration<double>(5));

		std::cout << "Disarming..." << std::endl;
		client->armDisarm(false, vehicleName);

		std::cout << "Done!..." << std::endl;

		delete client;

		std::this_thread::sleep_for(std::chrono::duration<double>(50));
	}
	catch (rpc::rpc_error&  e)
	{
		std::string msg = e.get_error().as<std::string>();
		std::cout << "Exception raised by the API, something went wrong." << std::endl << msg << std::endl;
	}
}

int main(int argc, char *argv[])
{
	using namespace msr::airlib;

	uint16_t rpcPort = 41451;
	int numPlatforms = 1;

	std::cout << "argc is " << argc << std::endl;
	if (argc > 1)
	{
		std::cout << "Num plats string: " << argv[1] << std::endl;
		int numPlatsInt = atoi(argv[1]);
		numPlatforms = static_cast<uint16_t>(numPlatsInt);
	}

	std::cout << "First port is " << rpcPort << std::endl;
	std::cout << "Num platforms: " << numPlatforms << std::endl;
	std::cout << "Making clients..." << std::endl;


	try
	{
		std::cout << "Press Enter to begin..." << std::endl; std::cin.get();

		std::vector<std::thread> clientThreads;

		// Count down, so the first one can easily go the highest (without knowing count)
		int clientOrdinal = numPlatforms - 1;
		for (int i = 0; i < numPlatforms; i++)
		{
			clientThreads.push_back(std::thread(runSingleClient, rpcPort, clientOrdinal));
			clientOrdinal--;

			std::this_thread::sleep_for(std::chrono::duration<double>(0.1));
		}

		for (auto &toJoin : clientThreads)
		{
			toJoin.join();
		}
	}
	catch (rpc::rpc_error&  e)
	{
		std::string msg = e.get_error().as<std::string>();
		std::cout << "Exception raised by the API, something went wrong." << std::endl << msg << std::endl;
	}

	return 0;
}
