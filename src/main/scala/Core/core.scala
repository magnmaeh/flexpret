/******************************************************************************
File: core.scala
Description: FlexPRET Processor (configurable 5-stage RISC-V processor)
Author: Michael Zimmer (mzimmer@eecs.berkeley.edu)
Contributors: Edward Wang (edwardw@eecs.berkeley.edu)
License: See LICENSE.txt
******************************************************************************/
package flexpret.core

import chisel3._
import chisel3.util.log2Ceil
import chisel3.util.MixedVec
import chisel3.experimental.chiselName

// Remove this eventually
import Core._
import Core.FlexpretConstants._


class InstMemBusIO(implicit conf: FlexpretConfiguration) extends Bundle {
  // read/write port
  val addr = Input(UInt(conf.iMemAddrBits.W))
  val enable = Input(Bool())
  val data_out = Output(UInt(32.W))
  val write = Input(Bool())
  val data_in = Input(UInt(32.W))
  val ready = Output(Bool()) // doesn't have priority
}

class DataMemBusIO(implicit conf: FlexpretConfiguration) extends Bundle {
  // read/write port
  val addr = Input(UInt((conf.dMemAddrBits - 2).W)) // assume word aligned
  val enable = Input(Bool())
  val data_out = Output(UInt(32.W))
  val byte_write = Input(Vec(4, Bool()))
  val data_in = Input(UInt(32.W))
}

class BusIO(implicit conf: FlexpretConfiguration) extends Bundle {
  val addr = Input(UInt(conf.busAddrBits.W)) // assume word aligned
  val enable = Input(Bool())
  val data_out = Output(UInt(32.W))
  val write = Input(Bool())
  val data_in = Input(UInt(32.W))
}

class HostIO() extends Bundle {
  val to_host = Output(UInt(32.W))
}

class GPIO(implicit conf: FlexpretConfiguration) extends Bundle {
  val in = MixedVec(conf.gpiPortSizes.map(i => Input(UInt(i.W))).toSeq)
  val out = MixedVec(conf.gpoPortSizes.map(i => Output(UInt(i.W))).toSeq)
}

class CoreIO(implicit val conf: FlexpretConfiguration) extends Bundle {
  val imem_core = if (conf.imemConfig.bypass) Some(Flipped(new InstMemCoreIO)) else None
  val imem_bus = new InstMemBusIO
  val dmem = new DataMemBusIO()
  val bus = Flipped(new BusIO())
  val host = new HostIO()
  val gpio = new GPIO()
  val int_exts = Input(Vec(8, Bool()))
  //val int_exts = Input(Vec(conf.threads, Bool()))
}

@chiselName
class Core(val confIn: FlexpretConfiguration) extends Module {
  implicit val conf = confIn

  // Write flexpret_config.h to file
  conf.writeConfigHeaderToFile("programs/lib/include/flexpret_config.h")

  val io = IO(new CoreIO)

  val control = Module(new Control())
  val datapath = Module(new Datapath())
  val imem = if (conf.imemConfig.bypass) None else Some(Module(new ISpm()))
  val dmem = Module(new DSpm())
  //val dmem = Module(new DSpm_BRAM())

  // internal
  datapath.io.control <> control.io
  datapath.io.imem <> (imem match {
    case Some(imem_module) => imem_module.io.core
    case _ => io.imem_core.get
  })
  datapath.io.dmem <> dmem.io.core

  // external
  io.imem_bus <> (imem match {
    case Some(imem_module) => imem_module.io.bus
    case _ => DontCare
  })
  io.dmem <> dmem.io.bus
  io.bus <> datapath.io.bus
  io.host <> datapath.io.host
  io.gpio <> datapath.io.gpio
  for (tid <- 0 until conf.threads) {
    datapath.io.int_exts(tid) := io.int_exts(tid)
  }
  //io.int_exts <> datapath.io.int_exts

}
