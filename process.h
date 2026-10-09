#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Состояние процесса
enum class ProcessState {
  // создан, но ещё не поступил
  NEW,
  // готов к выполнению, ждёт в очереди
  READY,
  // выполняется на CPU
  RUNNING,
  // заблокирован по вводу-выводу
  WAITING,
  // процесс завершён
  TERMINATED
};

// Точка блокировки по вводу-выводу
struct IoBlock {
  // на каком такте своей работы процесс уходит в I/O
  std::uint64_t atTick;
  // сколько тактов ждёт
  std::uint64_t duration;
};

// Описание процесса
struct Process {
  int pid = 0;
  std::string name;

  // Основные времена
  // момент поступления
  std::uint64_t arrivalTime = 0;
  // полное время CPU
  std::uint64_t burstTime = 0;
  // сколько ещё осталось
  std::uint64_t remainingTime = 0;
  // чем меньше, тем выше приоритет
  int priority = 0;
  // список I/O-блокировок
  std::vector<IoBlock> ioBlocks;

  // Метрики (заполняются симулятором)
  // момент первого запуска

  // запускался ли процесс хотя бы раз
  bool started = false;
  
  std::uint64_t startTime = 0;
  // момент завершения
  std::uint64_t finishTime = 0;
  // суммарное время в READY
  std::uint64_t waitingTime = 0;
  // finish - arrival
  std::uint64_t turnaroundTime = 0;
  // start - arrival
  std::uint64_t responseTime = 0;
  // суммарное время в WAITING
  std::uint64_t ioWaitTime = 0;
  // сколько раз процесс получал CPU
  int contextSwitches = 0;

  // Внутреннее состояние
  ProcessState state = ProcessState::NEW;
  // сколько уже отработал (для I/O)
  std::uint64_t executedTicks = 0;
  // когда вернётся из I/O
  std::uint64_t ioReturnTick = 0;
  // индекс следующей блокировки
  std::size_t nextIoIndex = 0;
  // динамический приоритет (для старения)
  int dynamicPriority = 0;
  // текущая очередь (для MLFQ)
  int queueLevel = 0;

  bool isFinished() const { return remainingTime == 0; }
};
