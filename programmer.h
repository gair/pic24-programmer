#if defined ARDUINO_NRF52832_FEATHER
#define MCLR      16
#define DATA      15
#define CLK       7
#elif defined ARDUINO_AVR_NANO
#define MCLR      7
#define DATA      9
#define CLK       8
#else
#error No port configuration for this board type
#endif

#define SIX       0
#define REGOUT    1
#define APPID     0xCB
#define REG_DEVID 0xff0000
#define REG_APPID 0x8007f0
#define EXEC_ADDR 0x800000
#define ERASE_BLOCK_SIZE 0x400

#define SCHECK    0x0 // Sanity Check
#define READC     0x1 // Read an 8-bit word from the specified Device ID register
#define READP     0x2 // Read N 24-bit instruction words of code memory starting from the specified address
#define PROGC     0x4 // Write an 8-bit word to the specified Device ID registers
#define PROGP     0x5 // Program One Row (64) 24 bit words of Code Memory and Verify
#define PROGW     0xd // Program One Word of Code Memory and Verify
#define QBLANK    0xa // Query if the Code Memory is Blank
#define QVER      0xb // Query the Programming Executive software version
#define ERASEB    0x7 // Bulk erase entire chip (Undocumented and not always working) 
#define ERASEP    0x9 // Erase page(s) (Undocumented but working) 
#define CRCP      0xc // CRC-16 calculation (Undocumented, untested) 

#define PASS      1
#define FAIL      2
#define NACK      3

// serial commands and responses
#define CmdError            "ERROR"
#define CmdReset            "RESET"
#define CmdDataOut          "DATAOUT:"
#define CmdTextOut          "TEXTOUT:"
#define CmdInitialize       "INITIALIZE"
#define CmdBlankCheck       "BLANKCHECK"
#define CmdReadWord         "READWORD"
#define CmdReadPage         "READPAGE"
#define CmdEraseExec        "ERASEEXEC"
#define CmdEraseBlocks      "ERASEBLOCKS"
#define CmdEraseChip        "ERASECHIP"
#define CmdEnterICSP        "ENTERICSP"
#define CmdEnterEICSP       "ENTEREICSP"
#define CmdExitICSP         "EXITICSP"
#define CmdDebugEnable      "DEBUGON"
#define CmdDebugDisable     "DEBUGOFF"
#define CmdRequestApp       "REQUESTAPP"
#define CmdRequestExec      "REQUESTEXEC"
#define CmdSendPage         "SENDPAGE"
#define CmdLoadApp          "LOADAPP"
#define CmdLoadExec         "LOADEXEC"
#define CmdVerifyApp        "VERIFYAPP"
#define CmdVerifyExec       "VERIFYEXEC"
#define CmdVerifyFail       "VERIFYFAIL"
#define CmdExecVersion      "EXECVERSION"
#define CmdDeviceID         "DEVICEID"
#define CmdAppID            "APPID"
#define CmdResetMCU         "RESETMCU"
#define CmdMemoryAddr       "MEMORYADDR"
#define CmdMemorySize       "MEMORYSIZE"
#define CmdNumPages         "NUMPAGES"
#define CmdNoPages          "NOPAGES"
#define CmdWrongMode        "WRONGMODE"
#define CmdMemoryCRC        "MEMORYCRC"

typedef union exec_resp
{
  uint16_t header;
  struct
  {
    uint16_t qe_code  : 8;
    uint16_t last_cmd : 4;
    uint16_t opcode   : 4;
    uint16_t length;
  };
} resp_t;
