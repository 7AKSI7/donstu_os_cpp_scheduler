#include "simulator.h"

SimResult runSimulation(Scheduler& sched, std::uint64_t maxTicks) {
  SimResult res;
  res.algorithm = sched.name();

  std::uint64_t tick = 0;
  int currentPid = -1;
  int prevPid = -1;
  std::uint64_t busyTicks = 0;

  auto& procs = sched.processes();

  while (tick < maxTicks) {
    // 1. Возвращаем процессы из I/O
    for (auto& p : procs) {
      if (p.state == ProcessState::WAITING && p.ioReturnTick <= tick) {
        p.state = ProcessState::READY;
        sched.onProcessReady(p.pid, tick);
      }
    }

    // 3. Все ли завершены?
    bool allDone = true;
    for (auto& p : procs) {
      if (p.state != ProcessState::TERMINATED) { allDone = false; break; }
    }
    if (allDone) break;

    // 4. Планировщик обновляет очереди
    sched.onTick(tick);

    // 5. Вытеснение
    if (currentPid != -1 && sched.shouldPreempt(currentPid, tick)) {
      Process* p = sched.find(currentPid);
      if (p && !p->isFinished() && p->state == ProcessState::RUNNING) {
        p->state = ProcessState::READY;
        sched.onProcessPreempted(currentPid, tick);
      }
      currentPid = -1;
    }

    // 6. Выбор нового процесса
    if (currentPid == -1) {
      currentPid = sched.pickNext(tick);
      if (currentPid != -1) {
        Process* p = sched.find(currentPid);
        if (p) {
          if (!p->started) {
            p->started = true;
            p->startTime = tick;
            p->responseTime = tick - p->arrivalTime;
          }
          p->state = ProcessState::RUNNING;
          p->contextSwitches++;
          if (prevPid != currentPid) res.contextSwitches++;
        }
      }
    }

    // 7. Выполняем один такт
    int ranPid = -1;
    if (currentPid != -1) {
      Process* p = sched.find(currentPid);
      if (p) {
        // запомним, кто реально выполнялся в этом такте
        ranPid = currentPid;

        p->remainingTime--;
        p->executedTicks++;
        busyTicks++;
        sched.onProcessRanTick(currentPid);

        // Проверка I/O-блокировки
        if (p->nextIoIndex < p->ioBlocks.size() &&
            p->executedTicks == p->ioBlocks[p->nextIoIndex].atTick) {
          p->state = ProcessState::WAITING;
          p->ioReturnTick = tick + 1 + p->ioBlocks[p->nextIoIndex].duration;
          p->ioWaitTime += p->ioBlocks[p->nextIoIndex].duration;
          p->nextIoIndex++;
          sched.onProcessBlocked(currentPid, tick);
          currentPid = -1;
        }
        // Проверка завершения
        else if (p->isFinished()) {
          p->finishTime = tick + 1;
          p->turnaroundTime = p->finishTime - p->arrivalTime;
          sched.onProcessFinished(currentPid, tick);
          currentPid = -1;
        }
      }
    }


     // 7a. Все, кто остался в READY, ждали этот такт

    for (auto& p : procs) {
      if (p.state == ProcessState::READY) {
      p.waitingTime++;
      }
    }

    // 8. Запись в диаграмму Ганта (по фактически выполнявшемуся процессу)
    if (ranPid != -1) {
      if (!res.gantt.empty() && res.gantt.back().first == ranPid) {
        res.gantt.back().second.second = tick + 1;
      } else {
        res.gantt.push_back({ranPid, {tick, tick + 1}});
      }
    } else {
      // CPU простаивал
      bool anyAlive = false;
      for (auto& p : procs) {
        if (p.state != ProcessState::TERMINATED) { anyAlive = true; break; }
      }
      if (anyAlive) {
        if (!res.gantt.empty() && res.gantt.back().first == -1) {
          res.gantt.back().second.second = tick + 1;
        } else {
          res.gantt.push_back({-1, {tick, tick + 1}});
        }
      }
    }

    prevPid = ranPid;
    tick++;
  }

  // Метрики
  double sumW = 0, sumT = 0, sumR = 0;
  int finished = 0;
  for (auto& p : procs) {
    if (p.state == ProcessState::TERMINATED) {
      sumW += p.waitingTime;
      sumT += p.turnaroundTime;
      sumR += p.responseTime;
      finished++;
    }
  }
  if (finished > 0) {
    res.avgWaiting = sumW / finished;
    res.avgTurnaround = sumT / finished;
    res.avgResponse = sumR / finished;
  }
  res.totalTicks = tick;
  res.cpuUtilization = tick > 0 ? 100.0 * busyTicks / tick : 0.0;
  res.throughput = finished;

  return res;
}
