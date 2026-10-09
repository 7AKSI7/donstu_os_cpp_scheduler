#pragma once
#include "process.h"
#include "simulator.h"
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
// Таблица по каждому процессу (вызывать после runSimulation)

inline void printProcessTable(const std::vector<Process>& procs) {
	std::cout << std::left << std::setw(6) << "Proc" << std::right
		<< std::setw(8) << "arrival" << std::setw(7) << "burst"
		<< std::setw(8) << "start" << std::setw(8) << "finish"
		<< std::setw(7) << "wait" << std::setw(7) << "turn"
		<< std::setw(7) << "resp" << std::setw(7) << "io" << "\n";
	for (const auto& p : procs) {
		std::cout << std::left << std::setw(6) << p.name << std::right
			<< std::setw(8) << p.arrivalTime << std::setw(7) << p.burstTime
			<< std::setw(8) << p.startTime << std::setw(8) << p.finishTime
			<< std::setw(7) << p.waitingTime << std::setw(7) << p.turnaroundTime
			<< std::setw(7) << p.responseTime << std::setw(7) << p.ioWaitTime
			<< "\n";
	}
}
