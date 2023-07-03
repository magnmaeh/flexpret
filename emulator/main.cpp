/*
 * main.cpp
 * C++ main entry point for Verilator simulation.
 *
 * Copyright 2021 Edward Wang <edwardw@eecs.berkeley.edu>
 */
#include "VVerilatorTop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <iostream>
#include <fstream>

uint64_t timestamp = 0;

double sc_time_stamp() {
  return timestamp;
}

int main(int argc, char* argv[]) {
  bool trace_enabled = false;
  bool measure_enabled = false;

  for (int i = 1; i< argc; i++) {
    if (!strcmp(argv[i], "--trace")) {
      std::cout << "Tracing enabled" << std::endl;
      trace_enabled = true;
    }

    if (!strcmp(argv[i], "--measure")) {
      std::cout << "Measure enabled" << std::endl;
      measure_enabled = true;
    }
  }

  Verilated::commandArgs(argc, argv);
  Verilated::traceEverOn(true);


  VVerilatorTop *top = new VVerilatorTop;
  VerilatedVcdC *trace;
  if (trace_enabled) {
    trace = new VerilatedVcdC;
    top->trace(trace, 99);
    trace->open("trace.vcd");
  }

  std::ofstream measure;
  if (measure_enabled) {
    measure = std::ofstream ("measure.out", std::ofstream::app);
  }

  while (!Verilated::gotFinish()) {
    // Hold reset high the two first clock cycles.
    if (timestamp <= 2) {
      top->reset = 1;
    } else {
      top->reset = 0;
    }

    top->clock = 1;
    top->eval();
    timestamp++;

    if (trace_enabled) {
      trace->dump(10*timestamp);
    }

    top->clock = 0;
    top->eval();
    timestamp++;

    if (trace_enabled) {
      trace->dump(10*timestamp);
      trace->flush();
    }

    if(top->io_stop == 1) {
      break;
    }
  }

  if (trace_enabled) {
    trace->close();
    delete trace;
  }
  if (measure_enabled) {
    measure << timestamp;
    measure.close();
  }

  delete top;
  return 0;
}
