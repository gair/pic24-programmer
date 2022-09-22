
#define TBLPAG 0x32
#define NVMCON 0x760
#define VISI   0x784

#define W0     0
#define W1     1
#define W2     2
#define W3     3
#define W4     4
#define W5     5
#define W6     6
#define W7     7
#define W8     8
#define W9     9
#define W10    10
#define W11    11
#define W12    12
#define W13    13
#define W14    14
#define W15    15
#define WR     15

typedef enum
{
    DIRECT = 0,
    INDIRECT = 1,
    POST_DECREMENT = 2,
    POST_INCREMENT = 3,
    PRE_DECREMENT = 4,
    PRE_INCREMENT = 5
} AddrMode;

void NOP();
void CLR(uint8_t wreg, AddrMode addr_mode);
void BSET(uint16_t addr, uint8_t bit4);
void TBLWTL(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLWTLB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLWTH(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLWTHB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLRDL(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLRDLB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLRDH(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void TBLRDHB(uint8_t wreg_src, AddrMode src_mode, uint8_t wreg_dest, AddrMode dest_mode);
void MOVI(uint16_t data, uint8_t wreg);
void MOVW(uint8_t wreg, uint16_t addr);
void MOVF(uint16_t addr, uint8_t wreg);
void GOTO(uint16_t here);
