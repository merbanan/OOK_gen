/*
Copyright (C) 2015 Benjamin Larsson <benjamin@southpole.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#define OOK_PIN 15
#define LED_PIN 17

#define MOD_PPM 1
#define MOD_PWM 0

char inChar;
String inputString        = "";         // a string to hold incoming data
unsigned int modulation   = 0;          // PWM = 0, PPM = 1
unsigned int repeats      = 5;          // signal repeats
unsigned int bits         = 36;         // amount of bits in a packet
unsigned int pd_len       = 2000;       // pulse/distance length (in us)
unsigned int zero_ca_len  = 250;        // length of 0 (in us)
unsigned int zero_ci_len  = 1250;       // length of 0 (in us)
unsigned int one_ca_len   = 1250;       // length of 1 (in us)
unsigned int one_ci_len   = 250;        // length of 0 (in us)
unsigned int pause_len    = 10000;      // pause length (in us), time between packets
unsigned int preamble     = 2500;       // preamble length (in us)
unsigned int invert       = 0;          // invert the bits before transmit
char packet_buf[256]      = {0};        // packet payload buffer
unsigned int pbuf_len     = 0;          // payload buffer length

unsigned int bit_pos      = 0;          // bit reader bit position
int carrier_mode          = 0;
void setup()
{
  // initialize digital pin 13 as an output.
  pinMode(OOK_PIN, OUTPUT);  // ook transmitter
  pinMode(LED_PIN, OUTPUT);  // led
  
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  
  // start serial port at 9600 bps and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("OOK_gen 0.1");
  Serial.println("Copyright (C) 2015");  
  Serial.println("by Benjamin Larsson");
  Serial.print("> ");
}

char hextoInt(char hex_nibble) {
  switch (hex_nibble) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 0xA;
    case 'B': return 0xB;
    case 'C': return 0xC;
    case 'D': return 0xD;
    case 'E': return 0xE;
    case 'F': return 0xF;
    case 'a': return 0xA;
    case 'b': return 0xB;
    case 'c': return 0xC;
    case 'd': return 0xD;
    case 'e': return 0xE;
    case 'f': return 0xF;
    default: return 0;
  }
}

// sprintf bugs made me do this, the object code is smaller without sprintf also
char get_hex_char(char hchar){
  if (hchar>9)
    return hchar+'A'-10;
  else
    return hchar+'0';
}

void printhex(char hex_char){
  char tmp;
  tmp = get_hex_char(((hex_char>>4)&0x0F));
  Serial.print(tmp);
  tmp = get_hex_char(hex_char&0x0F);
  Serial.print(tmp);
  Serial.print(" ");
}

int get_bit() {
  int ret;
  int byte_pos     = bit_pos / 8;
  int byte_bit_pos = 7 - (bit_pos % 8);     // reverse indexing to send the bits msb
  bit_pos++;
  ret = (packet_buf[byte_pos] & (1<<byte_bit_pos)) ? 1 : 0;
  return ret^invert;
}

int transmit_signal() {
  int i,j;
  int bit;
  int pwm_bl;

  // send preamble - not implemented

  // support leds on Leonardo and Pro Micro
  if (LED_PIN == 13)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  // repeats
  for (j=0 ; j<repeats ; j++) {
    // reset bit reader
    bit_pos = 0;

    // send bits
    for (i=0 ; i<bits ; i++) {
      bit = get_bit();
      if ((modulation==MOD_PPM) || (modulation==MOD_PWM)) {
        digitalWrite(OOK_PIN, HIGH);
        if (bit) {
          delayMicroseconds(one_ca_len);
          digitalWrite(OOK_PIN, LOW);
          delayMicroseconds(one_ci_len);
        } else {
          delayMicroseconds(zero_ca_len);
          digitalWrite(OOK_PIN, LOW);
          delayMicroseconds(zero_ci_len);
        }
      } else {
        return -1; 
      }
    }
    
    // Send ending PPM pulse
    if (modulation == MOD_PPM) {
        digitalWrite(OOK_PIN, HIGH);
//        digitalWrite(LED_PIN, HIGH);
        delayMicroseconds(pd_len);
        digitalWrite(OOK_PIN, LOW);
//        digitalWrite(LED_PIN, LOW);    
    }
    // delay between packets
    delayMicroseconds(pause_len);
  }

  // support leds on Leonardo and Pro Micro
  if (LED_PIN == 13)
    digitalWrite(LED_PIN, LOW);
  else
    digitalWrite(LED_PIN, HIGH);

  return 0;
}

void loop()
{
  // read serial input
  if (Serial.available() > 0) {
    inChar = (char)Serial.read();
    inputString += inChar;

    // Serial echo
    Serial.print(inChar);


    // handle commands
    if (inChar=='\r') {

      Serial.print("\nCMD: ");
      Serial.println(inputString);

      switch((char)inputString[0]) {
        case 'm':
          if (inputString.length() == 2) {
            Serial.print("Modulation: ");
            if (modulation == MOD_PPM)
              Serial.println("PPM");
            else if (modulation == MOD_PWM)
              Serial.println("PWM");
            else
              Serial.println("Invalid state");            
          }
          if (inputString.length() > 3) {
            modulation = (char)inputString[2]-'0';   // the easy way
          }
          break;
        case 'i':
          if (inputString.length() == 2) {
            Serial.print("Invert: ");
            Serial.println(invert);
          }
          if (inputString.length() > 3) {
            invert = (char)inputString[2]-'0';   // the easy way
          }
          break;
        case 'r':
          if (inputString.length() == 2) {
            Serial.print("Repeats: ");
            Serial.println(repeats);
          }
          if (inputString.length() > 3) {
            repeats = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'b':
          if (inputString.length() == 2) {
            Serial.print("Bits: ");
            Serial.println(bits);
          }
          if (inputString.length() > 3) {
            bits = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'z':
          if (inputString.length() == 2) {
            Serial.print("Zero ca length: ");
            Serial.println(zero_ca_len);
          }
          if (inputString.length() > 3) {
            zero_ca_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'x':
          if (inputString.length() == 2) {
            Serial.print("Zero ci length: ");
            Serial.println(zero_ci_len);
          }
          if (inputString.length() > 3) {
            zero_ci_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'o':
          if (inputString.length() == 2) {
            Serial.print("One ca length: ");
            Serial.println(one_ca_len);
          }
          if (inputString.length() > 3) {
            one_ca_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'u':
          if (inputString.length() == 2) {
            Serial.print("One ci length: ");
            Serial.println(one_ci_len);
          }
          if (inputString.length() > 3) {
            one_ci_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;

        case 'd':
          if (inputString.length() == 2) {
            Serial.print("Pulse / Distance length(pd/z/o): ");
            Serial.println(pd_len);
          }
          if (inputString.length() > 3) {
            pd_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'e':
          if (inputString.length() == 2) {
            Serial.print("Pause length: ");
            Serial.println(pause_len);
          }
          if (inputString.length() > 3) {
            pause_len = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'a':
          if (inputString.length() == 2) {
            Serial.print("Preamble length: ");
            Serial.println(preamble);
          }
          if (inputString.length() > 3) {
            preamble = inputString.substring(2,inputString.length()).toInt();  // the hard way
          }
          break;
        case 'p':
          if (inputString.length() == 2) {
            Serial.print("Packet: ");
            Serial.println(pbuf_len);
            for (int i=0 ; i<pbuf_len ; i++){
              printhex(packet_buf[i]);
            }
            Serial.print("\n");
          }
          if (inputString.length() > 3) {
            for (int i=0 ; i<inputString.length()-3 ; i++){
              packet_buf[i]  = hextoInt((char)inputString[(i*2) + 2]) << 4;
              packet_buf[i] |= hextoInt((char)inputString[(i*2) + 3]);
            }
            // TODO clear the packet_buf buffer 
            pbuf_len = ((inputString.length()-3)+1)/2;  //round up
          }
          break;
        case 't':
            {
            int res = transmit_signal();
            if (!res)
              Serial.println("OK");
            else {
              Serial.print("FAIL: ");
              Serial.println(res);
            }
            }
          break;
        case 's':
          if (inputString.length() == 2) {
            Serial.print("Carrier mode: ");
            Serial.println(carrier_mode);
          }
          if (inputString.length() > 3) {
            carrier_mode = inputString[2]-'0';   // the easy way
            if (carrier_mode)
              digitalWrite(OOK_PIN, HIGH);
            else
              digitalWrite(OOK_PIN, LOW);
          }
          break;
        }
      // reset command line buffer
      inputString = "";
      Serial.print("> ");
    }
  }
}

