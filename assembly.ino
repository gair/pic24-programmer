#include "programmer.h"

void NOP()
{
  uint32_t op = 0x0;
  writeOp(op);
}

void CLR(uint8_t wreg, AddrMode addr_mode)
{
  uint32_t op = 0xEB;
  op <<= 5;
  op |= addr_mode & 0x7;
  op <<= 4;
  op |= wreg & 0xf;
  op <<= 7;
  writeOp(op);
}

void BSET(uint16_t addr, uint8_t bit4)
{
  uint32_t op = 0xA8;
  op <<= 4;
  op |= bit4 & 0xe;
  op <<= 12;
  op |= addr & 0x1ffe;
  op |= bit4 & 0x1;
  writeOp(op);
}

void TBLWTL(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLWTL_Base(wreg_src, src_mode, wreg_dest, dest_mode, 0);
}

void TBLWTLB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLWTL_Base(wreg_src, src_mode, wreg_dest, dest_mode, 1);
}

void TBLWTL_Base(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode, uint8_t byte_mode)
{
  uint32_t op = 0xBB0;
  op |= (byte_mode & 0x1) << 2;
  op <<= 4;
  op |= (dest_mode & 0x7) << 3;
  op <<= 4;
  op |= (wreg_dest & 0xf) << 3;
  op |= src_mode & 0x7;
  op <<= 4;
  op |= wreg_src & 0xf;
  writeOp(op);
}

void TBLWTH(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLWTH_Base(wreg_src, src_mode, wreg_dest, dest_mode, 0);
}

void TBLWTHB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLWTH_Base(wreg_src, src_mode, wreg_dest, dest_mode, 1);
}

void TBLWTH_Base(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode, uint8_t byte_mode)
{
  uint32_t op = 0xBB8;
  op |= (byte_mode & 0x1) << 2;
  op <<= 4;
  op |= (dest_mode & 0x7) << 3;
  op <<= 4;
  op |= (wreg_dest & 0xf) << 3;
  op |= src_mode & 0x7;
  op <<= 4;
  op |= wreg_src & 0xf;
  writeOp(op);
}

void TBLRDL(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLRDL_Base(wreg_src, src_mode, wreg_dest, dest_mode, 0);
}

void TBLRDLB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLRDL_Base(wreg_src, src_mode, wreg_dest, dest_mode, 1);
}

void TBLRDL_Base(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode, uint8_t byte_mode)
{
  uint32_t op = 0xBA0;
  op |= (byte_mode & 0x1) << 2;
  op <<= 4;
  op |= (dest_mode & 0x7) << 3;
  op <<= 4;
  op |= (wreg_dest & 0xf) << 3;
  op |= src_mode & 0x7;
  op <<= 4;
  op |= wreg_src & 0xf;
  writeOp(op);
}

void TBLRDH(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLRDH_Base(wreg_src, src_mode, wreg_dest, dest_mode, 0);
}

void TBLRDHB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode)
{
  TBLRDH_Base(wreg_src, src_mode, wreg_dest, dest_mode, 1);
}

void TBLRDH_Base(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode, uint8_t byte_mode)
{
  uint32_t op = 0xBA8;
  op |= (byte_mode & 0x1) << 2;
  op <<= 4;
  op |= (dest_mode & 0x7) << 3;
  op <<= 4;
  op |= (wreg_dest & 0xf) << 3;
  op |= src_mode & 0x7;
  op <<= 4;
  op |= wreg_src & 0xf;
  writeOp(op);
}

void MOVI(uint16_t data, uint8_t wreg)
{
  uint32_t op = 0x2;
  op <<= 16;
  op |= data;
  op <<= 4;
  op |= wreg & 0xf;
  writeOp(op);
}

void MOVW(uint8_t wreg, uint16_t addr)
{
  uint32_t op = 0x88;
  op <<= 12;
  op |= addr >> 1;
  op <<= 4;
  op |= wreg & 0xf;
  writeOp(op);
}

void MOVF(uint16_t addr, uint8_t wreg)
{
  uint32_t op = 0x80;
  op <<= 12;
  op |= addr >> 1;
  op <<= 4;
  op |= wreg & 0xf;
  writeOp(op);
}

void GOTO(uint16_t here)
{
  uint32_t op = 0x4;
  writeOp(op << 16 | here & 0xfff7);
}
