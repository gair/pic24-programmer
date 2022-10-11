#include "assembly.h"
#include "programmer.h"

bool in_icsp = false;
bool in_eicsp = false;
bool debugging = false; 
uint32_t page_buffer[64];
const uint32_t page_buffer_size = sizeof(page_buffer) / sizeof(page_buffer[0]);
uint16_t code_buffer[(page_buffer_size * 3) / 2 + 2];

void setup() 
{
  pinMode(MCLR, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  exitICSP();
  
  Serial.begin(115200);
  Serial.println(CmdReset);
}

void loop()
{      
  uint32_t data;
  bool ok;
  String command = readSerial();

  if(command == CmdInitialize)
  {
    debugging = false;
    exitICSP();
    Serial.println(CmdInitialize);
  }
  else if(command == CmdBlankCheck) {
    ok = memoryBlankCheck();
    Serial.print(CmdDataOut);
    Serial.println(ok ? 1 : 0);
    Serial.println(CmdBlankCheck);
  }
  else if(command == CmdReadWord) {
    data = readMemoryWord();
    Serial.print(CmdDataOut);
    Serial.println(data);
    Serial.println(CmdReadWord);
  }
  else if(command == CmdReadPage) {
    readMemoryPage();
    Serial.println(CmdReadPage);
  }
  else if(command == CmdEraseBlocks) {
    ok = eraseMemoryBlocks();
    Serial.println(ok ? CmdEraseBlocks : CmdError);
  }
  else if(command == CmdEraseChip) {
    ok = eraseAllUserMemory();
    Serial.println(ok ? CmdEraseChip : CmdError);
  }
  else if(command == CmdEraseExec) {
    eraseExecBlock();
    Serial.println(CmdEraseExec);
  }
  else if(command == CmdEnterICSP) {
    ok = enterICSP(false);
    Serial.println(ok ? CmdEnterICSP : CmdError);
  }
  else if(command == CmdEnterEICSP) {
    ok = enterICSP(true);
    Serial.println(ok ? CmdEnterEICSP : CmdError);
  }
  else if(command == CmdExitICSP) {
    exitICSP();
    Serial.println(CmdExitICSP);
  }
  else if(command == CmdDebugEnable) {
    debugging = true;
    Serial.println(in_eicsp ? "In EICSP" : (in_icsp ? "In ICSP" : ""));
    Serial.println(CmdDebugEnable);
  }
  else if(command == CmdDebugDisable) {
    debugging = false;
    Serial.println(CmdDebugDisable);
  }
  else if(command == CmdRequestApp)
  {
    Serial.println(CmdRequestApp); 
  }
  else if(command == CmdRequestExec)
  {
    Serial.println(CmdRequestExec); 
  }
  else if(command == CmdLoadExec)
  {
    ok = loadProgramExecutive();
    Serial.println(ok ? CmdLoadExec : CmdError); 
  }
  else if(command == CmdLoadApp)
  {
    ok = loadApplication();
    Serial.println(ok ? CmdLoadApp : CmdError); 
  }
  else if(command == CmdVerifyApp)
  {
    ok = verifyApplication();
    Serial.println(ok ? CmdVerifyApp : CmdError);
  }
  else if(command == CmdVerifyExec)
  {
    ok = verifyProgramExecutive();
    Serial.println(ok ? CmdVerifyExec : CmdError);
  }
  else if(command == CmdExecVersion)
  {
    data = readExecVersion();
    Serial.print(CmdDataOut);
    Serial.println(data, DEC);
    Serial.println(CmdExecVersion);
  }
  else if(command == CmdDeviceID)
  {
    data = readDevId();
    Serial.print(CmdDataOut);
    Serial.println(data, DEC);
    Serial.println(CmdDeviceID);
  }
  else if(command == CmdAppID)
  {
    data = readAppId();
    Serial.print(CmdDataOut);
    Serial.println(data, DEC);
    Serial.println(CmdAppID);
  }
  else if(command == CmdMemoryCRC)
  {
    ok = calculateMemoryCRC();
    if (!ok) {
      Serial.println(CmdError);
    }
    else {
      Serial.print(CmdDataOut);
      Serial.println(code_buffer[0], DEC);
      Serial.println(CmdMemoryCRC);
    }
  }
  else if(command == CmdResetMCU)
  {
    reset();
    Serial.println(CmdResetMCU);
  }
}

// ICSP functions

bool calculateMemoryCRC() {
  if(in_eicsp) {
    Serial.println(CmdMemoryAddr);
    String hexAddr = readSerial();
    uint32_t address = strtoul(hexAddr.c_str(), 0, 16);
    Serial.println(CmdMemorySize);
    String hexSize = readSerial();
    uint32_t size = strtoul(hexSize.c_str(), 0, 16);
    return calculateCRC(address, size, code_buffer);
  } else {
    Serial.println(CmdWrongMode);
    return false;
  }
}

bool loadProgramExecutive()
{
  if(in_eicsp) {
    Serial.println(CmdWrongMode);
    return false;
  }
  eraseExecBlock();
  
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    
    progPage(address, page_buffer);
  }
  return true;
}

bool verifyProgramExecutive()
{
  int pageCount = 0;
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    if(!verifyPage(address, page_buffer)) {
      Serial.print(CmdDataOut);
      Serial.println(address, DEC);
      Serial.println(CmdVerifyFail);
      return false;
    }
    pageCount++;
  }
  return (pageCount == 16);
}

bool loadApplication()
{
  if(in_eicsp) {
    return loadApplicationEx();
  }
  
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }

    progPage(address, page_buffer);
  }
  return true;
}

bool verifyApplication()
{
  if(in_eicsp) {
    return verifyApplicationEx();
  }
  
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }

    if(!verifyPage(address, page_buffer)) {
      Serial.print(CmdDataOut);
      Serial.println(address, DEC);
      Serial.println(CmdVerifyFail);
      return false;
    }
  }
  return true;
}

void eraseExecBlock()
{
  if(in_eicsp) {
    erasePages(EXEC_ADDR, 2);
    return;
  }
  eraseBlocks(EXEC_ADDR, 2);
}

bool eraseMemoryBlocks()
{
  Serial.println(CmdMemoryAddr);
  String hexAddr = readSerial();
  uint32_t addr = strtoul(hexAddr.c_str(), 0, 16);
  Serial.println(CmdNumPages);
  String pages = readSerial();
  uint16_t numPages = strtoul(pages.c_str(), 0, 10);
  if (in_eicsp) {
    return erasePages(addr, numPages);
  }
  eraseBlocks(addr, numPages);
  return true;
}

bool eraseAllUserMemory()
{
  if (in_eicsp) {
    //return eraseChipEx(); // disabling as it's unreliable
    Serial.println(CmdWrongMode); 
    return false;
  }
  eraseChip();
  return true;
}

int readMemoryWord()
{
  Serial.println(CmdMemoryAddr);
  String hexAddr = readSerial();
  uint32_t addr = strtoul(hexAddr.c_str(), 0, 16);
  return readWord(addr);
}

void readMemoryPage() {
  if(in_eicsp) {
    readMemoryPageEx();
    return;
  }
  Serial.println(CmdMemoryAddr);
  String hexAddr = readSerial();
  uint32_t address = strtoul(hexAddr.c_str(), 0, 16);
  uint32_t offset = 0;
  
  for (int index = 0; index < 64; index += 2) {
    readMemory(address + offset, &page_buffer[index], &page_buffer[index + 1]);
    offset += 4;
  }
  
  sendPageHex();
}

void readMemory(uint32_t address, uint32_t *word0, uint32_t *word1)
{
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();
  
  // Initialize TBLPAG and the Read Pointer (W6) for the TBLRD instruction.
  MOVI((address >> 16) & 0xff, W0);
  MOVW(W0, TBLPAG);
  MOVI(address & 0xffff, W6);

  // Initialize the Write Pointer (W7) to point to the VISI register. 
  MOVI(VISI, W7);
  NOP();
  
  uint32_t lsw0, lsw1, msb;
  TBLRDL(W6, INDIRECT, W7, INDIRECT); 
  NOP();
  NOP();
  lsw0 = regout();
  NOP();
  TBLRDHB(W6, POST_INCREMENT, W7, POST_INCREMENT);
  NOP();
  NOP();
  TBLRDHB(W6, PRE_INCREMENT, W7, POST_DECREMENT);
  NOP();
  NOP();
  msb = regout();
  NOP();
  TBLRDL(W6, POST_INCREMENT, W7, INDIRECT);
  NOP();
  NOP();
  lsw1 = regout();
  NOP();
  // Reset the device internal PC. 
  GOTO(0x200);
  NOP();

  lsw0 |= (msb << 16) & 0xff0000;
  lsw1 |= (msb << 8) & 0xff0000;
  *word0 = lsw0;
  *word1 = lsw1;
}

bool verifyPage(uint32_t address, const uint32_t *data)
{
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();
  
  // Initialize TBLPAG and the Read Pointer (W6) for the TBLRD instruction.
  MOVI((address >> 16) & 0xff, W0);
  MOVW(W0, TBLPAG);
  MOVI(address & 0xffff, W6);

  // Initialize the Write Pointer (W7) to point to the VISI register. 
  MOVI(VISI, W7);
  NOP();
  
  uint32_t lsw0, lsw1, msb;
  for(int i = 0; i < 64; i += 2) {
    // Read and clock out the contents of the next two locations of executive memory through
    // the VISI register using the REGOUT command.
    TBLRDL(W6, INDIRECT, W7, INDIRECT); 
    NOP();
    NOP();
    lsw0 = regout();
    NOP();
    TBLRDHB(W6, POST_INCREMENT, W7, POST_INCREMENT);
    NOP();
    NOP();
    TBLRDHB(W6, PRE_INCREMENT, W7, POST_DECREMENT);
    NOP();
    NOP();
    msb = regout();
    NOP();
    TBLRDL(W6, POST_INCREMENT, W7, INDIRECT);
    NOP();
    NOP();
    lsw1 = regout();
    NOP();
    // Reset the device internal PC. 
    GOTO(0x200);
    NOP();

    lsw0 |= (msb << 16) & 0xff0000;
    lsw1 |= (msb << 8) & 0xff0000;

    if(data[i] != 0xffffffff && data[i] != lsw0) {
      return false;
    }
    if(data[i + 1] != 0xffffffff && data[i + 1] != lsw1) {
      return false;
    }
  }
  return true;
}

uint16_t readWord(uint32_t address)
{
  // Exit Reset vector
  NOP();
  GOTO(0x200);
  NOP();

  // Initialize TBLPAG and the Read Pointer (W0) for the TBLRD instruction.
  MOVI((address >> 16) & 0xff, W0);
  MOVW(W0, TBLPAG);
  MOVI(address & 0xffff, W0);
  MOVI(VISI, W1);
  NOP();
  TBLRDL(W0, INDIRECT, W1, INDIRECT);
  NOP();
  NOP();

  // Output the VISI register using the REGOUT command
  uint16_t data = regout(); // Clock out contents of the VISI register.
  NOP();
  return data;
}

void progPage(uint32_t address, uint32_t *data)
{
  // Load W0:W5 with the next four words of packed code and initialize W6 for
  // programming. Programming starts from address using W6 as a Read Pointer 
  // and W7 as a Write Pointer.

  // Initialize the NVMCON to program 64 instruction words.
  MOVI(0x4001, W0);
  MOVW(W0, NVMCON);

  // Initialize TBLPAG and the Write Pointer (W7).
  MOVI(address >> 16, W0);
  MOVW(W0, TBLPAG);
  NOP();
  MOVI(address & 0xffff, W7);
  NOP();
  
  int index = 0;
  for(int i = 0; i < 16; i++)
  {
    uint16_t lsw = data[index] & 0xffff;
    uint16_t msw = (data[index++] >> 16) & 0xff;
    msw |= (data[index] >> 8) & 0xff00;
    MOVI(lsw, W0);
    MOVI(msw, W1);
    lsw = data[index++] & 0xffff;
    MOVI(lsw, W2);
    lsw = data[index] & 0xffff;
    MOVI(lsw, W3);
    msw = (data[index++] >> 16) & 0xff;
    msw |= (data[index] >> 8) & 0xff00;
    MOVI(msw, W4);
    lsw = data[index++] & 0xffff;
    MOVI(lsw, W5);
    
    // Set the Read Pointer (W6) and load the (next four write) latches.
    CLR(W6, DIRECT);
    NOP();
    TBLWTL(W6, POST_INCREMENT, W7, INDIRECT);
    NOP();
    NOP();
    TBLWTHB(W6, POST_INCREMENT, W7, POST_INCREMENT);
    NOP();
    NOP();
    TBLWTHB(W6, POST_INCREMENT, W7, PRE_INCREMENT);
    NOP();
    NOP();
    TBLWTL(W6, POST_INCREMENT, W7, POST_INCREMENT);
    NOP();
    NOP();
    TBLWTL(W6, POST_INCREMENT, W7, INDIRECT);
    NOP();
    NOP();
    TBLWTHB(W6, POST_INCREMENT, W7, POST_INCREMENT);
    NOP();
    NOP();
    TBLWTHB(W6, POST_INCREMENT, W7, PRE_INCREMENT);
    NOP();
    NOP();
    TBLWTL(W6, POST_INCREMENT, W7, POST_INCREMENT);
    NOP();
    NOP();
  }
  // Initiate the programming cycle.
  BSET(NVMCON, WR);
  NOP();
  NOP();
  pollWR(W2);
   
  // Reset the device internal PC. 
  GOTO(0x200);
  NOP();
}

void eraseBlocks(uint32_t address, uint16_t numBlocks)
{
  eraseMemory(address, numBlocks, false);
}

void eraseChip()
{
  eraseMemory(0, 1, true);
}

void eraseMemory(uint32_t address, uint16_t numPages, bool chipErase)
{
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();
  int eraseTime = 40; // P12
  uint16_t mode = 0x4042;
  if (chipErase) {
    numPages = 1;
    mode = 0x404f;
    eraseTime = 400;  // P11
  }
  
  for(uint16_t page = 0; page < numPages; page++) {
    address += page * 0x400;
    MOVI(mode, W0);
    MOVW(W0, NVMCON);
    MOVI(address >> 16, W0);
    MOVW(W0, TBLPAG);
    MOVI(address & 0xffff, W1);
    NOP();
    TBLWTL(W1, DIRECT, W1, INDIRECT);
    NOP();
    NOP();
    BSET(NVMCON, WR);
    NOP();
    NOP();
    delay(eraseTime);
    pollWR(W2);
  }
}

void pollWR(uint8_t wreg)
{
  // Poll the WR bit (bit 15 of NVMCON) until it is cleared by the hardware.
  uint16_t wr_mask = 1 << WR;
  uint16_t data = wr_mask;
  while((data & wr_mask) != 0) {
    GOTO(0x200);
    NOP();
    MOVF(NVMCON, wreg);
    MOVW(wreg, VISI);
    NOP();
    data = regout();
    NOP();
  }
}

// Extended ICSP funtions


bool memoryBlankCheck()
{
  Serial.println(CmdMemorySize);
  String hexSize = readSerial();
  uint32_t size = strtoul(hexSize.c_str(), 0, 16);
  return blankCheck(size);
}

void readMemoryPageEx() {
  Serial.println(CmdMemoryAddr);
  String hexAddr = readSerial();
  uint32_t address = strtoul(hexAddr.c_str(), 0, 16);

  readCodeMemory(address, 64, code_buffer);

  int n = 0;
  for (int i = 0; i < 64; i += 2) {
    unpackTwoWords(&code_buffer[n], &page_buffer[i]);
    n += 3;
  }
  
  sendPageHex();
}

bool sanityCheck()
{
  resp_t resp;
  execCommand(SCHECK, 1, NULL, NULL, 1000, &resp);
  if(resp.opcode == PASS) {
    return (resp.length == 2);
  }
  return false;
}

bool blankCheck(uint32_t size)
{
  resp_t resp;
  uint16_t input[2];
  input[0] = size >> 16;
  input[1] = size & 0xffff;
  execCommand(QBLANK, 3, input, NULL, 700000, &resp);
  if(resp.opcode == PASS) {
    return (resp.qe_code == 0xf0);
  }
  return false;
}

uint16_t readExecVersion()
{
  resp_t resp;
  execCommand(QVER, 1, NULL, NULL, 1000, &resp);
  if(resp.opcode == PASS && resp.last_cmd == QVER) {
    return resp.qe_code;
  }
  return 0;
}

bool loadApplicationEx()
{
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    
    int n = 2;
    for (int i = 0; i < 64; i += 2) {
      packTwoWords(&page_buffer[i], &code_buffer[n]);
      n += 3;
    }
    
    if(!writeCodeMemory(address, 0, code_buffer)) {
      return false;
    }
  }
  return true;
}

bool verifyApplicationEx()
{
  while(true) {
    Serial.println(CmdSendPage);
    String page = readSerial();
    if (page == CmdNoPages) {
      break;
    }
    uint32_t address;
    if(!parseHexPage24(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    
    if(!readCodeMemory(address, 64, code_buffer)) {
      return false;
    }

    int n = 0;
    uint32_t unpacked[2]; 
    for (int i = 0; i < 64; i += 2) {
      unpackTwoWords(&code_buffer[n], unpacked);
      if(unpacked[0] != page_buffer[i] || unpacked[1] != page_buffer[i + 1]) {
        Serial.print(CmdDataOut);
        Serial.println(address, DEC);
        Serial.println(CmdVerifyFail);
        return false;
      }
      n += 3;
    }
  }
  return true;
}

bool readRegisters(uint32_t address, int count, uint16_t *output)
{
  resp_t resp;
  uint16_t input[2];
  input[0] = count << 8;
  input[0] |= address >> 16;
  input[1] = address & 0xffff;
  execCommand(READC, 3, input, output, 1000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == READC && resp.qe_code == 0);
}

bool writeRegister(uint32_t address, uint16_t data)
{
  resp_t resp;
  uint16_t input[3];
  input[0] = address >> 16;
  input[1] = address & 0xffff;
  input[2] = data;
  execCommand(PROGC, 4, input, NULL, 5000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == PROGC && resp.qe_code == 0);
}

bool readCodeMemory(uint32_t address, int words, uint16_t *output)
{
  resp_t resp;
  uint16_t input[3];
  input[0] = words;
  input[1] = address >> 16;
  input[2] = address & 0xffff;
  uint16_t rows = ((words / 64) + 1);
  execCommand(READP, 4, input, output, 1000 * rows, &resp);
  return(resp.opcode == PASS && resp.last_cmd == READP && resp.qe_code == 0);
}

bool writeCodeMemory(uint32_t address, int row, uint16_t *data)
{
  resp_t resp;
  address += row * 0x80;
  data[0] = address >> 16;
  data[1] = address & 0xffff;
  execCommand(PROGP, 0x63, data, NULL, 5000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == PROGP && resp.qe_code == 0);
}

bool writeCodeWord(uint32_t address, uint32_t data)
{
  resp_t resp;
  uint16_t input[3];
  input[0] = (data >> 8) & 0xff00;
  input[0] |= (address >> 16) & 0xff;
  input[1] = address & 0xffff;
  input[2] = data & 0xffff;
  execCommand(PROGW, 4, input, NULL, 5000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == PROGW && resp.qe_code == 0);
}

bool erasePages(uint32_t address, uint16_t numPages)
{
  resp_t resp;
  uint16_t input[2];
  input[0] = numPages << 8;
  input[0] |= (address >> 16) & 0xff;
  input[1] = address & 0xffff;
  execCommand(ERASEP, 3, input, NULL, 25000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == ERASEP && resp.qe_code == 0);
}

bool eraseChipEx()
{
  resp_t resp;
  uint16_t input[1];
  input[0] = 3;
  execCommand(ERASEB, 2, input, NULL, 125000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == ERASEB && resp.qe_code == 0);
}

bool calculateCRC(uint32_t address, uint32_t size, uint16_t *crc)
{
  resp_t resp;
  uint16_t input[4];
  input[0] = address >> 16;
  input[1] = address & 0xffff;
  input[2] = size >> 16;
  input[3] = size & 0xffff;
  execCommand(CRCP, 5, input, crc, 1000000, &resp);
  return(resp.opcode == PASS && resp.last_cmd == CRCP && resp.qe_code == 0);
}

void execCommand(uint8_t command, uint16_t length, uint16_t *input, uint16_t *output, uint32_t timeoutMicros, resp_t *resp)
{
  uint16_t data = command;
  data <<= 12;
  data |= length & 0xfff;
  writeExec(data, 16);
  for(int i = 1; i < length; i++) {
    writeExec(input[i - 1], 16);
  }
  delayMicroseconds(12); // P8 12us
  pinMode(DATA, INPUT);
  delayMicroseconds(40); // P9 40us
  while(digitalRead(DATA) == HIGH && timeoutMicros-- > 0) {
    delayMicroseconds(1);
  }
  if (timeoutMicros == 0) {
    resp->opcode = FAIL;
    return;
  }
  delayMicroseconds(23); // P20 23us
  resp->header = readExec();
  delayMicroseconds(8);  // P21 8ns?? typo in data sheet? Needs 8us not 8ns
  resp->length = readExec();
  if(resp->opcode == PASS) {
    for(int i = 2; i < resp->length; i++) {
      delayMicroseconds(8); // P21 8ns
      output[i - 2] = readExec();
    }
  }
  delayMicroseconds(1);
  pinMode(DATA, OUTPUT);
}

// Common utility functions

void packTwoWords(uint32_t *unpacked, uint16_t *packed)
{
  packed[0] = unpacked[0] & 0xffff;
  packed[1] = (unpacked[0] >> 16) & 0xff;
  packed[1] |= (unpacked[1] >> 8) & 0xff00;
  packed[2] = unpacked[1] & 0xffff;
}

void unpackTwoWords(uint16_t *packed, uint32_t *unpacked)
{
  unpacked[0] = packed[1] & 0xff;
  unpacked[0] <<= 16;
  unpacked[0] |= packed[0];
  unpacked[1] = packed[1] & 0xff00;
  unpacked[1] <<= 8;
  unpacked[1] |= packed[2];
}

uint16_t readDevId()
{
  return readId(REG_DEVID);
}

uint16_t readAppId()
{
  return readId(REG_APPID);
}

uint16_t readId(uint32_t address)
{
  if (in_eicsp) {
    uint16_t id;
    if(readRegisters(address, 1, &id)) {
      return id;
    }
    return 0;
  }
  return readWord(address);
}

bool enterICSP(bool enhanced)
{
  digitalWrite(MCLR, HIGH);
  digitalWrite(MCLR, LOW);
  delayMicroseconds(1); // P18 = 40ns
  uint32_t code = enhanced ? 0x4D434850 : 0x4D434851;
  writeExec(code, 32);

  digitalWrite(DATA, LOW);
  digitalWrite(CLK, LOW);

  delay(2);  // P19 = 1ms
  digitalWrite(MCLR, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(26); // P7 =  25ms
  if (enhanced) {
    in_icsp = true;
    in_eicsp = true;
    return sanityCheck();
  }
  uint32_t data = readDevId();
  return (data & 0xff00) == 0x4200; // PIC24 family
}

void exitICSP()
{
  digitalWrite(DATA, LOW);
  digitalWrite(CLK, LOW);
  digitalWrite(MCLR, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  in_icsp = false;
  in_eicsp = false;
}

void reset()
{
  digitalWrite(MCLR, LOW);
  delayMicroseconds(2);
  digitalWrite(MCLR, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  in_icsp = false;
  in_eicsp = false;
}

void write(uint32_t data, int bits)
{
  for (int i = 0; i < bits; i++)
  {
    writeBit((data >> i) & 1);
  }
}

void writeExec(uint32_t data, int bits)
{
  bits -= 1;
  for (int i = bits; i >= 0; i--)
  {
    writeBit((data >> i) & 1);
  }
  if (debugging) {
    Serial.println(data, HEX);
  }
}

uint16_t readExec()
{
  uint16_t b0, b1;
  // EICSP is MSb first
  for(int i = 15; i >= 0; i--)
  {
    b0 = readBit(true);
    b1 |= b0 << i;
  }
  if (debugging) {
    Serial.print("->");
    Serial.println(b1, HEX);
  }
  return b1;
}

void writeOp(uint32_t op)
{
  int bits = 4;
  if (!in_icsp) {
    in_icsp = true;
    bits += 5;                // 5 extra clocks on ICSP entry
  }
  write(SIX, bits);
  delayMicroseconds(1);       // P4 = 40ns
  write(op, 24);
  if (debugging) {
    Serial.println(op, HEX);
  }
}

uint16_t regout()
{
  uint16_t value = 0;
  write(REGOUT, 4);
  delayMicroseconds(1);       // P4 = 40ns
  write(0, 8);                // CPU idle 8 cycles
  pinMode(DATA, INPUT);
  delayMicroseconds(1);       // P5 = 20ns
  uint16_t b;
  // ICSP is LSb first
  for(int i = 0; i < 16; i++)
  {
    b = readBit(false);
    value |= b << i;
  }
  pinMode(DATA, OUTPUT);
  delayMicroseconds(1);       // P4A = 40ns
  if (debugging) {
    Serial.print("->");
    Serial.println(value, HEX);
  }
  return value;
}

void writeBit(int state)
{
  digitalWrite(DATA, state);
  digitalWrite(CLK, HIGH);    // Data change
  delayMicroseconds(1);       // P1B 40ns
  digitalWrite(CLK, LOW);     // Data write
  delayMicroseconds(1);       // P1A 40ns
}

byte readBit(bool fallingEdge)
{
  byte _bit;
  digitalWrite(CLK, HIGH);    // Change ICSP
  delayMicroseconds(1);       // P1B 40ns
  _bit = digitalRead(DATA);   // Read ICSP
  digitalWrite(CLK, LOW);     // Change EICSP
  if (fallingEdge) {
    delayMicroseconds(1);     // P1A 40ns
    _bit = digitalRead(DATA); // Read EICSP
  }
  delayMicroseconds(1);       // P1A 40ns
  return _bit;
}

// Serial data utilities

String readSerial() {
  while(Serial.available() == 0);
  String data = Serial.readStringUntil('\n');
  data.toUpperCase();
  data.trim();
  return data;
}

void serialResponse(String command, bool send) {
  if(send) {
    Serial.println(command);
  } else {
    Serial.print(command);
  }
}

bool parseHexPage24(String page, uint32_t *address, uint32_t *out_data, uint32_t size)
{
  const char sep = ' ';
  int start = 0;
  int length = page.length();
  int pos = page.indexOf(sep);
  if (pos > 0) {
    String addr = page.substring(start, pos);
    int col = addr.indexOf(':');
    if (col != -1) {
      addr[col] = '\0';
    }
    *address = strtoul(addr.c_str(), 0, 16);
    start = pos + 1;
    pos = page.indexOf(sep, start);
    int i = 0;
    bool done = false;
    while(!done) {
      if(i >= size || start >= length) {
        return false;
      }
      if(pos == -1) {
        done = true;
        pos = length;
      }
      String s = page.substring(start, pos);
      out_data[i++] = strtoul(s.c_str(), 0, 16);
      start = pos + 1;
      pos = page.indexOf(sep, start);
    }
    return true;
  }
  return false;
}

void sendPageHex() {
  String hex = "";
  for (int i = 0; i < 64; i++)
  {
    hex += ' ' + String(page_buffer[i], HEX);
  }
  hex.trim();
  Serial.print(CmdTextOut);
  Serial.println(hex.c_str());
}
