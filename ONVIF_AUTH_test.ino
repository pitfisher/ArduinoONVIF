#include <SPI.h>
#include <Ethernet.h>
#include <string.h>
#include "sha1.h"
#include "base64.hpp"
#include "xml.h"

byte mac[] = {  0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server(192, 168, 11, 22);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

byte *nonce;
char *createTime;
char *username = "admin";
char *password;

unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

char *httpHeaderStatic = "POST /onvif/Media HTTP/1.1\r\nUser-Agent: Arduino/1.0\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
char *onvif_header = "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">";
char *onvif_openHeader = "<s:Header>";
char *onvif_closeHeader = "</s:Header>";
char *onvif_body = "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">";
char *onvif_command_GetCapabilities = "<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetSystemDateAndTime = "<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetNetworkInterfaces = "<GetNetworkInterfaces xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetProfiles = "<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>";
char *onvif_command_GetServices = "<GetServices xmlns=\"http://www.onvif.org/ver10/media/wsdl\"><IncludeCapability>false</IncludeCapability></GetServices>";
char *onvif_command_ContinuousMove = "<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>Profile_1</ProfileToken><Velocity><PanTilt x=\"0\" y=\"0\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>";
char *onvif_closer = "</s:Body></s:Envelope>";
char *onvif_command;

void setup() {
  static char *securityHeader;
  
  Serial.begin(115200);
  
  randomSeed(analogRead(0));

  // disable SD SPI
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  createTime = "2018-11-07T18:15:45.000Z";
  password = "Supervisor";
  nonce = getNonce();
  
  securityHeader = calculateHeaderSecurity(username,password,createTime,nonce);
    
  onvif_command = onvif_command_GetProfiles;
  
  Serial.println(F("Starting ethernet..."));
  if (!Ethernet.begin(mac)) Serial.println(F("failed"));
  else {
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.gatewayIP());
  }
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    // Make a HTTP request:
    client.print(httpHeaderStatic);
    client.println(strlen(onvif_header)+strlen(onvif_openHeader)+strlen(onvif_closeHeader)+strlen(securityHeader)+strlen(onvif_body)+strlen(onvif_command) + strlen(onvif_closer)); //calculate and send Content-Length of request
    client.println();
    client.print(onvif_header);
    client.print(onvif_openHeader);
    client.print(securityHeader);
    client.print(onvif_closeHeader);
//    client.print(onvif_security1);
//    client.print(username);
//    client.print(onvif_security2);
//    client.print(base64_digest);
//    client.print(onvif_security3);
//    client.print(base64_nonce);
//    client.print(onvif_security4);
//    client.print(createTime);
//    client.print(onvif_security5);
    client.print(onvif_body);
    client.print(onvif_command);
    client.print(onvif_closer);
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  beginMicros = micros();
}

void loop() {
  int len = client.available();
  if (len > 0) {
    byte buffer[80];
    if (len > 80) len = 80;
    client.read(buffer, len);
    if (printWebData) {
      Serial.write(buffer, len); // show in the serial monitor (slows some boards)
    }
    byteCount = byteCount + len;
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    endMicros = micros();
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    Serial.print("Received ");
    Serial.print(byteCount);
    Serial.print(" bytes in ");
    float seconds = (float)(endMicros - beginMicros) / 1000000.0;
    Serial.print(seconds, 4);
    float rate = (float)byteCount / seconds / 1000.0;
    Serial.print(", rate = ");
    Serial.print(rate);
    Serial.print(" kbytes/second");
    Serial.println();

    // do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
}

void printHash(uint8_t *hash) {
  int i;
  for (i=0; i<20; i++) {
    Serial.print("0123456789abcdef"[hash[i]>>4]);
    Serial.print("0123456789abcdef"[hash[i]&0xf]);
  }
  Serial.println();
}

void printNonce(byte nonce[]) {
  int i;
  for(i = 0; i<20; i++) {
    Serial.print(nonce[i],HEX);
  }
  Serial.println();
}

uint8_t *getDigest(byte nonce_t[],char *password,char *created){
  char chars[20];
  char *conc;
  memcpy(chars, nonce_t,20); 
  chars[20] = '\0';
  Sha1.init();
  conc = malloc(strlen(chars)+strlen(password)+strlen(created));
  strcpy(conc,chars);
  strcat(conc,created);
  strcat(conc,password);
  Sha1.print(conc);
  return Sha1.result();
}

byte *getNonce() {
  byte random_num;
  static byte random_arr[20];
  int i = 0;
  for(i=0;i<20;i++) {
    random_num = random(255);
    random_arr[i] = random_num;
  }
  return random_arr;
}

char *calculateHeaderSecurity(char *username,char *password,char *createTime, byte nonce_l[]) {
  static char* securityHeader;
  char *base64_digest;
  char *base64_nonce;

  static uint8_t *hash;
  
  char *onvif_security1 = "<Security s:mustUnderstand=\"1\" xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\"><UsernameToken><Username>";
  char *onvif_security2 = "</Username><Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">";
  char *onvif_security3 = "</Password><Nonce EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">";
  char *onvif_security4 = "</Nonce><Created xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">";
  char *onvif_security5 = "</Created></UsernameToken></Security>";
  
  base64_digest = malloc(encode_base64_length(20));
  base64_nonce = malloc(encode_base64_length(20));
  hash = getDigest(nonce_l,password,createTime);
  
  encode_base64(hash, 20, base64_digest);
  encode_base64(nonce, 20, base64_nonce);
  
  securityHeader = malloc(strlen(onvif_security1)+strlen(username)+strlen(onvif_security2)+strlen(base64_digest)+strlen(onvif_security3)+strlen(base64_nonce)+strlen(onvif_security4)+strlen(createTime)+strlen(onvif_security5));
  strcpy(securityHeader,onvif_security1);
  strcat(securityHeader,username);
  strcat(securityHeader,onvif_security2);
  strcat(securityHeader,base64_digest);
  strcat(securityHeader,onvif_security3);
  strcat(securityHeader,base64_nonce);
  strcat(securityHeader,onvif_security4);
  strcat(securityHeader,createTime);
  strcat(securityHeader,onvif_security5);
  return securityHeader;
}
