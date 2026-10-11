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

// Формат файла: одна строка — один процесс
// pid имя arrival burst priority deadline io
// где io — список "atTick:duration" через запятую, либо "-" если I/O нет.
inline bool saveSet(const std::string& path, const std::vector<Process>& procs) {
    std::ofstream out(path);
    if (!out) return false;
    for (const auto& p : procs) {
        out << p.pid << ' ' << p.name << ' ' << p.arrivalTime << ' ' << p.burstTime << ' '
            << p.priority << ' ' << p.deadline << ' ';
        if (p.ioBlocks.empty()) {
            out << '-';
        } else {
            for (std::size_t i = 0; i < p.ioBlocks.size(); ++i) {
                if (i) out << ',';
                out << p.ioBlocks[i].atTick << ':' << p.ioBlocks[i].duration;
            }
        }
        out << '\n';
    }
    return true;
}

inline bool loadSet(const std::string& path, std::vector<Process>& procs) {
	std::ifstream in(path);
	if (!in) return false;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::istringstream ss(line);
		Process p;
		std::string io;
		if (!(ss >> p.pid >> p.name >> p.arrivalTime >> p.burstTime >>
			p.priority >> p.deadline >> io))
			return false;
		p.remainingTime = p.burstTime;
		p.dynamicPriority = p.priority;
		if (io != "-") {
			std::istringstream is(io);
			std::string item;
			while (std::getline(is, item, ',')) {
				std::size_t colon = item.find(':');
				if (colon == std::string::npos) return false;
				IoBlock b{};
				b.atTick = std::stoull(item.substr(0, colon));
				b.duration = std::stoull(item.substr(colon + 1));
				p.ioBlocks.push_back(b);
			}
		}
		procs.push_back(p);
	}
	return true;
}

// Случайный набор процессов. Один и тот же seed всегда даёт один и тот же набор.
inline std::vector<Process> makeRandomSet(unsigned seed, int count) {
	std::mt19937 gen(seed);
	std::uniform_int_distribution<int> arrival(0, 30), burst(2, 15);
	std::uniform_int_distribution<int> prio(1, 5), coin(0, 99);
	std::vector<Process> procs;
	for (int i = 0; i < count; ++i) {
		Process p;
		p.pid = i + 1;
		p.name = "P" + std::to_string(i + 1);
		p.arrivalTime = static_cast<std::uint64_t>(arrival(gen));
		p.burstTime = static_cast<std::uint64_t>(burst(gen));
		p.remainingTime = p.burstTime;
		p.priority = prio(gen);
		p.dynamicPriority = p.priority;
		// примерно у трети процессов есть I/O: atTick возрастает и строго меньше burstTime
		if (p.burstTime >= 4 && coin(gen) < 35) {
			std::uint64_t at = 1 + static_cast<std::uint64_t>(gen() % 2);
			while (at < p.burstTime) {
				p.ioBlocks.push_back({at, 2 + static_cast<std::uint64_t>(gen() % 4)});
				at += 2 + static_cast<std::uint64_t>(gen() % 3);
			}
		}
		procs.push_back(p);
	}
	return procs;
}