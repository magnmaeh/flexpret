package flexpret.core
import chisel3._
import chisel3.util._
import Core.FlexpretConstants._


case class WishboneConfig(
  numDevices: Int = 1,
  numMasters: Int = 1
) {
  require(numDevices > 0)
  require(numMasters == 1)

  def nAddrBits = log2Ceil(numDevices)
  def nDataBits = 32
}

class WishboneIO(cfg: WishboneConfig) extends Bundle {
  val addr = Output(UInt(cfg.nAddrBits.W))
  val wrData = Output(UInt(cfg.nDataBits.W))
  val rdData = Input(UInt(cfg.nDataBits.W))
  val we = Output(Bool())
  val sel = Output(UInt(4.W))
  val stb = Output(Bool())
  val ack = Input(Bool())
  val cyc = Output(Bool())

  def driveReadReq(addr: UInt): Unit = {
    addr := addr
    wrData := 0.U
    we := false.B
    cyc := true.B
    sel := 15.U// Assume we always want 4 bytes for now
    stb := true.B
  }

  def driveWriteReq(addr: UInt, data: UInt) = {
    addr := addr
    wrData := data
    we := true.B
    cyc := true.B
    sel := 15.U // Assume we always want 4 bytes for now
    stb := true.B
  }

  def driveDefaultFromMaster(): Unit = {
    addr := 0.U
    wrData := 0.U
    we := false.B
    cyc := false.B
    sel := 0.U
    stb := 0.U
  }
}

class WishboneFlexpretMaster(wbCfg: WishboneConfig)(implicit conf: FlexpretConfiguration) extends MultiIOModule {
  val wbIO = IO(new WishboneIO(wbCfg))
  val busIO = IO(new BusIO())

  wbIO.driveDefaultFromMaster()
  busIO.driveDefaultFromBus()

  // Registers with 1CC access latency from FlexPret core
  val regAddr = RegInit(0.U(wbCfg.nAddrBits.W))
  val regWriteData = RegInit(0.U(wbCfg.nDataBits.W))
  val regReadData = RegInit(0.U(wbCfg.nDataBits.W))
  val regStatus = RegInit(false.B)

  // Use a single register to delay read-out for a single cycle
  val regBusRead = RegInit(0.U(32.W))
  busIO.data_out := regBusRead

  val wDoRead = WireDefault(false.B)
  val wDoWrite = WireDefault(false.B)
  assert(!(wDoRead && wDoWrite), "Both read and write at the same time")

  // Handle read/write transactions from core
  when(busIO.enable) {
    val addr = busIO.data_in
    when(busIO.write) {
      // Handle writes
      wDoWrite := true.B
      when(addr === MMIO_READ_ADDR) {
          regAddr := busIO.data_in
        }.elsewhen(addr === MMIO_READ_ADDR) {
          regAddr := busIO.data_in
        }.elsewhen(addr === MMIO_WRITE_DATA) {
          regWriteData := busIO.data_in
        }.otherwise {
          assert(false.B, s"Tried to write to invalid address $addr on wishbone bus master")
      }
    }.otherwise {
      // Handle reads
      wDoRead := true.B
      when(addr === MMIO_READ_DATA) {
        regBusRead := regReadData
      }.elsewhen(addr === MMIO_STATUS) {
        regBusRead := regStatus
        regStatus := false.B
      }.otherwise {
        assert(false.B, s"Tried to read from invalid address $addr on wishbone bus master")
      }
    }
  }

  // Simple, un-optimized implementation of the wishbone protocol
  val sIdle :: sDoWrite :: sDoRead :: Nil = Enum(3)
  val regState = RegInit(sIdle)

  switch(regState) {
    // Idle state. Waiting for request from FlexPret Core
    //  Decouples the FlexPret load/store instructions from
    //  accessing the WB bus
    is (sIdle) {
      when (wDoRead) {
        regState := sDoRead
      }.elsewhen(wDoWrite) {
        regState := sDoWrite
      }
      assert(!wbIO.ack, "WBm in idle mode and recived ACK")
    }

    // Perform read operation. Drive read signals until we get an ack
    is (sDoRead) {
      wbIO.driveReadReq(regAddr)
      regStatus := false.B
      when(wbIO.ack) {
        regReadData := wbIO.rdData
        regStatus := true.B
        regState := sIdle
      }
    }

    // Perform write operation. Drive write signals until we get an ack
    is (sDoWrite) {
      wbIO.driveWriteReq(regAddr, regWriteData)
      regStatus := false.B
      when(wbIO.ack) {
        regStatus := true.B
        regState := sIdle
      }
    }
  }
}