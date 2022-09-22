#include "assembly.h"
#include "programmer.h"

#ifdef ARDUINO_NRF52832_FEATHER
#define MCLR 16
#define DATA 7
#define CLK  15
#else // Arduino Nano
#define MCLR 7
#define DATA 8
#define CLK  9
#endif

#define SIX    0
#define REGOUT 1
#define APPID  0xCB

#define SCHECK 0x0 // Sanity Check
#define READC  0x1 // Read an 8-bit word from the specified Device ID register
#define READP  0x2 // Read N 24-bit instruction words of code memory starting from the specified address
#define PROGC  0x4 // Write an 8-bit word to the specified Device ID registers
#define PROGP  0x5 // Program One Row (64) 24 bit words of Code Memory and Verify
#define PROGW  0xd // Program One Word of Code Memory and Verify
#define QBLANK 0xa // Query if the Code Memory is Blank
#define QVER   0xb // Query the Programming Executive software version

#define PASS 1
#define FAIL 2
#define NACK 3

bool in_icsp = false;
bool debugging = false; 
uint32_t page_buffer[64];
const uint32_t page_buffer_size = sizeof(page_buffer) / sizeof(page_buffer[0]);

typedef union resp
{
  struct
  {
    uint16_t qe_code : 8;
    uint16_t last_cmd : 4;
    uint16_t opcode : 4;
    uint16_t length;
  };
  uint16_t header;
} resp_t;

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
  else if(command == "EXIT") {
    exitICSP();
    Serial.println("EXITED_ICSP");
  }
  else if(command == "APP_ID") {
    data = readAppId();
    Serial.print("APP_ID=0x");
    Serial.println(data, HEX);
  }
  else if(command == "TEST") {
    data = readTest();
    Serial.print("TEST=");
    Serial.println(data ? "PASS" : "FAIL");
  }
  else if(command == "PROGRAM")
  {
    data = readAppId();
    if (data != APPID) {
      Serial.println("SEND_EXEC");
      return;
    }
    Serial.println("EXEC_LOADED"); 
  }
  else if(command == "LOADEXEC")
  {
    ok = loadProgrammingExecutive();
    Serial.println(ok ? "EXEC_LOADED" : "ERROR"); 
  }
  else if(command == "VERIFYEXEC")
  {
    ok = verifyProgrammingExecutive();
    Serial.println(ok ? "EXEC_VERIFIED" : "ERROR");
  }
  else if(command == "DEVICEID")
  {
    data = readDevId();
    Serial.print("DATA_OUT:");
    Serial.println(data, DEC);
    Serial.println("DEVICE_ID");
  }
  else if(command == "E")
  {
    debugging = true;
    ok = enterICSP(true);
    Serial.println(ok ? "ENTERED_EICSP" : "ERROR");
  }
  else if(command == "X")
  {
    resp_t resp;
    resp = sendCommand(SCHECK, 1);
    Serial.print("SCHECK=0x");
    Serial.println(resp.opcode, HEX);
        resp = sendCommand(SCHECK, 1);
    Serial.print("SCHECK LEN=");
    Serial.println(resp.length, DEC);
    
    resp = sendCommand(QVER, 1);
    Serial.print("QVER=");
    Serial.print(resp.qe_code >> 4, HEX);
    Serial.print(".");
    Serial.println(resp.qe_code & 0xf, HEX);
  }
  else if(command == "D") {
    data = readDevId();
    Serial.print("DEV_ID=0x");
    Serial.println(data, HEX);
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

bool loadProgrammingExecutive()
{
  eraseExec();
  
  // Initialize the NVMCON to program 64 instruction words.
  MOVI(0x4001, W0);
  MOVW(W0, NVMCON);

  // Initialize TBLPAG and the Write Pointer (W7).
  MOVI(0x80, W0);
  MOVW(W0, TBLPAG);
  CLR(W7, DIRECT);
  NOP();

  while(true) {
    Serial.println("SEND_PAGE");
    String page = readSerial(true);
    if (page == "DONE") {
      break;
    }
    uint32_t address;
    if(!readPage(page, &address, page_buffer, page_buffer_size)) {
      return false;
    }
    progPage(address, page_buffer);
    
    // Initiate the programming cycle.
    BSET(NVMCON, WR);
    NOP();
    NOP();
    pollWR(W2);
     
    // Reset the device internal PC. 
    GOTO(0x200);
    NOP();
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
    if(!readPage(page, &address, page_buffer, page_buffer_size)) {
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

bool readPage(String page, uint32_t *address, uint32_t *data, uint32_t size)
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
      data[i] = temp << 24;
      data[i] |= ((temp << 8) & 0xff0000);
      data[i] |= ((temp >> 8) & 0xff00);
      data[i++] |= ((temp >> 24) & 0xff);
      start = pos + 1;
      pos = page.indexOf(sep, start);
    }
    
    return true;
  }
  return false;
}

char hexChar(const char *data) {
  return *data > '9' ? (*data | 0x20) + 10 - 'a' : *data - '0';
}

bool enterICSP(bool enhanced)
{
  digitalWrite(MCLR, HIGH);
  digitalWrite(MCLR, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delayMicroseconds(1); // P18 = 40ns
  uint32_t code = enhanced ? 0x4D434850 : 0x4D434851;
  writeICSP(code);

  digitalWrite(DATA, LOW);
  digitalWrite(CLK, LOW);

  delay(2);  // P19 = 1ms
  digitalWrite(MCLR, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(26); // P7 =  25ms
  if (enhanced) {
    in_icsp = true;
    resp_t r = sendCommand(SCHECK, 1);
    return r.opcode == PASS && r.length == 2;
  }
  uint32_t data = readDevId();
  return data >= 0x4202 && data <= 0x420f;
}

void exitICSP()
{
  digitalWrite(DATA, LOW);
  digitalWrite(CLK, LOW);
  digitalWrite(MCLR, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  in_icsp = false;
}

bool readTest()
{
  // Exit Reset vector
  NOP();
  GOTO(0x200);
  NOP();
  
  // Load data into VISI
  const uint16_t TEST_DATA = 0x1234;
  MOVI(TEST_DATA, W0);
  MOVW(W0, VISI);
  NOP();

  // Output the VISI register
  uint16_t data = regout();
  NOP();
  return data == TEST_DATA;
}

uint16_t readDevId()
{
  return readWord(0xff0000);
}

uint16_t readAppId()
{
  return readWord(0x8007f0);
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
  // Load W0:W5 with the next four words of packed Programming Executive code and initialize W6 for
  // programming. Programming starts from the base of executive memory (800000h) using W6 as a Read Pointer 
  // and W7 as a Write Pointer.

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
}

void eraseExec()
{
  // Exit the Reset vector and erase executive memory.
  NOP();
  GOTO(0x200);
  NOP();

  // Initialize the NVMCON to erase executive memory.
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

resp_t sendCommand(uint8_t command, uint16_t length)
{
  resp_t resp;
  uint32_t data = command;
  data <<= 12;
  data |= length & 0xfff;
  writeToExec(data, 16);
  delayMicroseconds(1);
  pinMode(DATA, INPUT);
  delayMicroseconds(12); // P8 12us
  delayMicroseconds(40); // P9 40us
  delayMicroseconds(23); // P20 23us
  resp.header = readFromExec();
  delayMicroseconds(1); // P21 8ns
  resp.length = readFromExec();
  delayMicroseconds(1);
  pinMode(DATA, OUTPUT);
  return resp;
}

void writeToExec(uint32_t data, int bits)
{
  bits -= 1;
  for (int i = bits; i >= 0; i--)
  {
    writeBit((data >> i) & 1);
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

void writeICSP(uint32_t data)
{
  writeToExec(data, 32);
  if (debugging) {
    Serial.println(data, HEX);
  }
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
