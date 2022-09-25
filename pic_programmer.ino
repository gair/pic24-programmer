#include "assembly.h"
#include "programmer.h"

bool in_icsp = false;
bool in_eicsp = false;
bool debugging = false; 
uint32_t page_buffer[64];
const uint32_t page_buffer_size = sizeof(page_buffer) / sizeof(page_buffer[0]);

void setup() 
{
  pinMode(MCLR, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  exitICSP();
  
  Serial.begin(9600);
  Serial.println("RESET");
}

void loop()
{      
  uint32_t data;
  bool ok;
  String command = readSerial(true);

  if(command == "START_DEBUG") {
    command = "START";
    debugging = true;
  }
  
  if(command == "START")
  {
      ok = enterICSP(false);
      Serial.println(ok ? "ENTERED_ICSP" : "ERROR");
  }
  else if(command == "ENTERICSP") {
    ok = enterICSP(false);
    Serial.println(ok ? "ENTERED_ICSP" : "ERROR");
  }
  else if(command == "DEBUGON") {
    debugging = true;
    Serial.println("DEBUG_ON");
  }
  else if(command == "DEBUGOFF") {
    debugging = false;
    Serial.println("DEBUG_OFF");
  }
  else if(command == "ENTEREICSP") {
    ok = enterICSP(true);
    Serial.println(ok ? "ENTERED_EICSP" : "ERROR");
  }
  else if(command == "EXITICSP") {
    exitICSP();
    Serial.println("EXITED_ICSP");
  }
  else if(command == "APP_ID") {
    data = readAppId();
    Serial.print("APP_ID=0x");
    Serial.println(data, HEX);
  }
  else if(command == "PROGRAM")
  {
    data = readAppId();
    if (data != APPID) {
      Serial.println("SEND_EXEC");
      return;
    }
    Serial.println("SEND_APP"); 
  }
  else if(command == "LOADEXEC")
  {
    ok = loadProgrammingExecutive();
    Serial.println(ok ? "EXEC_LOADED" : "ERROR"); 
  }
  else if(command == "LOADAPP")
  {
    ok = loadApplication();
    Serial.println(ok ? "APP_LOADED" : "ERROR"); 
  }
  else if(command == "VERIFYAPP")
  {
    ok = verifyApplication();
    Serial.println(ok ? "APP_VERIFIED" : "ERROR");
  }
  else if(command == "VERIFYEXEC")
  {
    ok = verifyProgrammingExecutive();
    Serial.println(ok ? "EXEC_VERIFIED" : "ERROR");
  }
  else if(command == "EXECVERSION")
  {
    uint16_t ver = versionCheck();
    Serial.print("DATA_OUT:");
    Serial.print(ver >> 4, HEX);
    Serial.print(".");
    Serial.println(ver & 0xf, HEX);
    Serial.println("EXEC_VERSION");
  }
  else if(command == "DEVICEID")
  {
    data = readDevId();
    Serial.print("DATA_OUT:");
    Serial.println(data, DEC);
    Serial.println("DEVICE_ID");
  }
  else if(command == "APPID")
  {
    data = readAppId();
    Serial.print("DATA_OUT:");
    Serial.println(data, DEC);
    Serial.println("APP_ID");
  }
  else if(command == "E")
  {
    debugging = true;
    ok = enterICSP(true);
    Serial.println(ok ? "ENTERED_EICSP" : "ERROR");
  }
  else if(command == "X")
  {
    bool ok = sanityCheck();
    Serial.print("SCHECK=");
    Serial.println(ok);

    uint16_t ver = versionCheck();
    Serial.print("QVER=");
    Serial.print(ver >> 4, HEX);
    Serial.print(".");
    Serial.println(ver & 0xf, HEX);
  }
  else if(command == "D") {
    data = readDevId();
    Serial.print("DEV_ID=0x");
    Serial.println(data, HEX);
  }
  else if(command == "B") {
    // 0057FEh (11K) 32
    // 00ABFEh (22K) 64
    // 5600
    ok = blankCheck(0x5600);
    Serial.print("QBLANK=");
    Serial.println(ok);
  }
  else if(command == "M") {
    uint16_t mem[10] = {0};
    ok = readCodeMemory(0, 3, mem);
    if(ok) {
      Serial.println(mem[0], HEX);
      Serial.println(mem[1], HEX);
      Serial.println(mem[2], HEX);
      Serial.println(mem[3], HEX);
      Serial.println(mem[4], HEX);
      Serial.println(mem[5], HEX);
    }
  }
  else if(command == "W") {
    ok = writeCodeWord(0, 0x123456);
    Serial.println(ok ? "OK" : "ERROR");
  }
  else if(command == "W-") {
    eraseBlock(0);
    Serial.println("EBLK");
  }
  else if(command == "EC") {
    eraseChip();
    Serial.println("Chip erased!");
  }
}

String readSerial(bool upperCase) {
  while(Serial.available() == 0);
  String data = Serial.readString();
  if (upperCase) {
    data.toUpperCase();
  }
  data.trim();
  return data;
}

bool loadApplication()
{
  while(true) {
    Serial.println("SEND_PAGE");
    String page = readSerial(true);
    if (page == "DONE") {
      break;
    }
    uint32_t address;
    if(!parseHexPage(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    if ((address & 0xffff) % BLOCK_SIZE == 0) {
      Serial.println("Erase block");
      eraseBlock(address);
      Serial.println("Erased");
    }

    Serial.println("Program page");
    progPage(address, page_buffer);
    Serial.println("Programmed");
  }
  return true;
}

bool verifyApplication()
{
  int pageCount = 0;
  while(true) {
    Serial.println("SEND_PAGE");
    String page = readSerial(true);
    if (page == "DONE") {
      break;
    }
    uint32_t address;
    if(!parseHexPage(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }

    if(!verifyPage(address, page_buffer)) {
      return false;
    }
  }
  return true;
}

bool loadProgrammingExecutive()
{
  eraseExec();
  
  while(true) {
    Serial.println("SEND_PAGE");
    String page = readSerial(true);
    if (page == "DONE") {
      break;
    }
    uint32_t address;
    if(!parseHexPage(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    
    progPage(address, page_buffer);
  }
  return true;
}

bool verifyProgrammingExecutive()
{
  int pageCount = 0;
  while(true) {
    Serial.println("SEND_PAGE");
    String page = readSerial(true);
    if (page == "DONE") {
      break;
    }
    uint32_t address;
    if(!parseHexPage(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }

    if(!verifyPage(address, page_buffer)) {
      return false;
    }
    pageCount++;    
  }
  return (pageCount == 16);
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

bool parseHexPage(String page, uint32_t *address, uint32_t *out_data, uint32_t size)
{
  const char sep = ' ';
  int start = 0;
  int length = page.length();
  int pos = page.indexOf(sep);
  if (pos > 0) {
    String addr = page.substring(start, pos);
    *address = strtoul(addr.c_str(), 0, 16) >> 1;
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
      uint32_t temp = strtoul(s.c_str(), 0, 16);
      out_data[i] = temp << 24;
      out_data[i] |= ((temp << 8) & 0xff0000);
      out_data[i] |= ((temp >> 8) & 0xff00);
      out_data[i++] |= ((temp >> 24) & 0xff);
      start = pos + 1;
      pos = page.indexOf(sep, start);
    }
    return true;
  }
  return false;
}

bool enterICSP(bool enhanced)
{
  digitalWrite(MCLR, HIGH);
  digitalWrite(MCLR, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delayMicroseconds(1); // P18 = 40ns
  uint32_t code = enhanced ? 0x4D434850 : 0x4D434851;
  writeToExec(code, 32);

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
  uint16_t id = regout(); // Clock out contents of the VISI register.
  NOP();
  return id;
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
  
  int index = 0;
  for(int i = 0; i < 16; i++)
  {
    uint16_t lsw = data[index] & 0xffff;
    uint16_t msw = (data[index++] >> 16) & 0xff;
    msw |= (data[index] >> 8) & 0xff00;
    MOVI(lsw, W0);
    MOVI(msw, W1);
    lsw = (data[index++]) & 0xffff;
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

void eraseExec()
{
  eraseBlock(0x80000);
  eraseBlock(0x80400);
  /*
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();

  // Initialize the NVMCON to erase memory.
  MOVI(0x4042, W0);
  MOVW(W0, NVMCON);

  // Initialize Erase Pointers to the page of the executive and then initiate the erase cycle.
  for(uint16_t addr = 0; addr <= 0x400; addr += 0x400) {
    MOVI(0x80, W0);
    MOVW(W0, TBLPAG);
    MOVI(addr, W1);
    NOP();
    TBLWTL(W1, DIRECT, W1, INDIRECT);
    NOP();
    NOP();
    BSET(NVMCON, WR);
    NOP();
    NOP();
    pollWR(W2);
  }
  */
}

void eraseBlock(uint32_t address)
{
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();

  // Initialize the NVMCON to erase memory.
  MOVI(0x4042, W0);
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
  pollWR(W2);
}

void eraseChip()
{
  // Exit the Reset vector.
  NOP();
  GOTO(0x200);
  NOP();

  // Initialize the NVMCON to erase all memory.
  MOVI(0x404f, W0);
  MOVW(W0, NVMCON);
  MOVI(0x00, W0);
  MOVW(W0, TBLPAG);
  MOVI(0x0000, W0);
  TBLWTL(W0, DIRECT, W0, INDIRECT);
  NOP();
  NOP();
  BSET(NVMCON, WR);
  NOP();
  NOP();
  delay(400);
  pollWR(W2);
}

void pollWR(uint8_t wreg)
{
  // Poll the WR bit (bit 15 of NVMCON) until it is cleared by the hardware.
  uint16_t wr = 1 << WR;
  uint16_t data = wr;
  while((data & wr) != 0) {
    GOTO(0x200);
    NOP();
    MOVF(NVMCON, wreg);
    MOVW(wreg, VISI);
    NOP();
    data = regout();
    NOP();
  }
}

void write(uint32_t data, int bits)
{
  for (int i = 0; i < bits; i++)
  {
    writeBit((data >> i) & 1);
  }
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
  execCommand(QBLANK, 3, input, NULL, 100000, &resp);
  if(resp.opcode == PASS) {
    return (resp.qe_code == 0xf0);
  }
  return false;
}

uint16_t versionCheck()
{
  resp_t resp;
  execCommand(QVER, 1, NULL, NULL, 1000, &resp);
  if(resp.opcode == PASS) {
    return resp.qe_code;
  }
  return 0;
}

bool readRegisters(uint32_t address, int count, uint16_t *output)
{
  resp_t resp;
  uint16_t input[2];
  input[0] = count << 8;
  input[0] |= address >> 16;
  input[1] = address & 0xffff;
  execCommand(READC, 3, input, output, 1000, &resp);
  return(resp.opcode == PASS);
}

bool writeRegister(uint32_t address, uint16_t data)
{
  resp_t resp;
  uint16_t input[3];
  input[0] = address >> 16;
  input[1] = address & 0xffff;
  input[2] = data;
  execCommand(PROGC, 4, input, NULL, 5000, &resp);
  return(resp.opcode == PASS);
}

bool readCodeMemory(uint32_t address, int count, uint16_t *output)
{
  resp_t resp;
  uint16_t input[3];
  input[0] = count;
  input[1] = address >> 16;
  input[2] = address & 0xffff;
  execCommand(READP, 4, input, output, 1000 * count, &resp);
  return(resp.opcode == PASS);
}

bool writeCodeMemory(uint32_t address, int row, uint16_t *data)
{
  resp_t resp;
  address += row * 0x80;
  data[0] = address >> 16;
  data[1] = address & 0xffff;
  execCommand(PROGP, 0x63, data, NULL, 5000, &resp);
  return(resp.opcode == PASS);
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
  return(resp.opcode == PASS);
}

void execCommand(uint8_t command, uint16_t length, uint16_t *input, uint16_t *output, uint32_t timeoutMicros, resp_t *resp)
{
  uint16_t data = command;
  data <<= 12;
  data |= length & 0xfff;
  writeToExec(data, 16);
  for(int i = 1; i < length; i++) {
    writeToExec(input[i - 1], 16);
  }
  delayMicroseconds(1);
  pinMode(DATA, INPUT);
  delayMicroseconds(12); // P8 12us
  delayMicroseconds(40); // P9 40us
  while(digitalRead(DATA) == HIGH && timeoutMicros-- > 0) {
    delayMicroseconds(1);
  }
  delayMicroseconds(23); // P20 23us
  resp->header = readFromExec();
  delayMicroseconds(8);  // P21 8ns?? typo in data sheet? Needs 8us not ns
  resp->length = readFromExec();
  if(resp->opcode == PASS) {
    for(int i = 2; i < resp->length; i++) {
      delayMicroseconds(8); // P21 8ns
      output[i - 2] = readFromExec();
    }
  }
  delayMicroseconds(1);
  pinMode(DATA, OUTPUT);
}

void writeToExec(uint32_t data, int bits)
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

uint16_t readFromExec()
{
  uint16_t b0, b1;
  for(int i = 15; i >= 0; i--)
  {
    b0 = readBit();
    b1 |= b0 << i;
  }
  if (debugging) {
    Serial.println(b1, HEX);
  }
  return b1;
}

void writeOp(uint32_t op)
{
  int bits = 4;
  if (!in_icsp) {
    in_icsp = true;
    bits = 9;
  }
  write(SIX, bits);
  delayMicroseconds(1); // P4 = 40ns
  write(op, 24);
  if (debugging) {
    Serial.println(op, HEX);
  }
}

uint16_t regout()
{
  uint16_t value = 0;
  write(REGOUT, 4);
  delayMicroseconds(1); // P4 = 40ns
  write(0, 8);          // CPU idle 8 cycles
  pinMode(DATA, INPUT);
  delayMicroseconds(1); // P5 = 20ns
  uint16_t b;
  for(int i = 0; i < 16; i++)
  {
    b = readBit();
    value |= b << i;
  }
  pinMode(DATA, OUTPUT);
  delayMicroseconds(1); // P4A = 40ns
  if (debugging) {
    Serial.println(value, HEX);
  }
  return value;
}

void writeBit(int state)
{
  digitalWrite(DATA, state);
  digitalWrite(CLK, HIGH);
  delayMicroseconds(1); // P1B 40ns
  digitalWrite(CLK, LOW); // clocked out
  delayMicroseconds(1); // P1A 40ns
}

byte readBit()
{
  byte _bit;
  digitalWrite(CLK, HIGH); // clocked in
  delayMicroseconds(1); // P1B 40ns
  _bit = digitalRead(DATA);
  digitalWrite(CLK, LOW);
  delayMicroseconds(1); // P1A 40ns
  return _bit;
}
