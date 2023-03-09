#include "flexpret_io.h"

#include <stdint.h>

// Write a generic value to the tohost CSR
inline void write_tohost(uint32_t val) { write_csr(CSR_TOHOST, val); }

// Print the given value in the simulation
inline void _fp_print(uint32_t val) {
  write_csr(CSR_TOHOST, 0xbaaabaaa);
  write_csr(CSR_TOHOST, val);
}

// Finish/stop the simulation
inline void _fp_finish() { write_csr(CSR_TOHOST, 0xdeaddead); }

// GPO ports, if port width < 32, then upper bits ignored
// CSR_GPO_*
// Write all GPO bits
inline void gpo_write_0(uint32_t val) { write_csr(CSR_UARCH4, val); }
inline void gpo_write_1(uint32_t val) { write_csr(CSR_UARCH5, val); }
inline void gpo_write_2(uint32_t val) { write_csr(CSR_UARCH6, val); }
inline void gpo_write_3(uint32_t val) { write_csr(CSR_UARCH7, val); }

// For each '1' bit in mask, set corresponding GPO bit to '1'
inline void gpo_set_0(uint32_t mask) { set_csr(CSR_UARCH4, mask); }
inline void gpo_set_1(uint32_t mask) { set_csr(CSR_UARCH5, mask); }
inline void gpo_set_2(uint32_t mask) { set_csr(CSR_UARCH6, mask); }
inline void gpo_set_3(uint32_t mask) { set_csr(CSR_UARCH7, mask); }

// For each '1' bit in mask, set corresponding GPO bit to '0'
inline void gpo_clear_0(uint32_t mask) { clear_csr(CSR_UARCH4, mask); }
inline void gpo_clear_1(uint32_t mask) { clear_csr(CSR_UARCH5, mask); }
inline void gpo_clear_2(uint32_t mask) { clear_csr(CSR_UARCH6, mask); }
inline void gpo_clear_3(uint32_t mask) { clear_csr(CSR_UARCH7, mask); }

// Read GPO bits
inline uint32_t gpo_read_0() { return read_csr(CSR_UARCH4); }
inline uint32_t gpo_read_1() { return read_csr(CSR_UARCH5); }
inline uint32_t gpo_read_2() { return read_csr(CSR_UARCH6); }
inline uint32_t gpo_read_3() { return read_csr(CSR_UARCH7); }

// GPI ports, if port width < 32, then upper bits are zero
// Read GPI bits
// CSR_GPI_*
inline uint32_t gpi_read_0() { return read_csr(CSR_UARCH0); }
inline uint32_t gpi_read_1() { return read_csr(CSR_UARCH1); }
inline uint32_t gpi_read_2() { return read_csr(CSR_UARCH2); }
inline uint32_t gpi_read_3() { return read_csr(CSR_UARCH3); }
