// #include "Arduino.h"
#include <Ethernet.h>
#include <ArduinoHttpClient.h>
#include "onvif.h"
#include <time.h>
#include "sha1.h"
#include "base64.hpp"

char *onvif_command_GetCapabilities = "<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetSystemDateAndTime = "<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetNetworkInterfaces = "<GetNetworkInterfaces xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetProfiles = "<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>";
char *onvif_command_GetServices = "<GetServices xmlns=\"http://www.onvif.org/ver10/media/wsdl\"><IncludeCapability>false</IncludeCapability></GetServices>";
char *onvif_command_ContinuousMove = "<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>Profile_1</ProfileToken><Velocity><PanTilt x=\"0\" y=\"0\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>";
char *httpHeaderDeviceService = "POST /onvif/device_service ";
char *httpHeaderPtzService = "POST /onvif/ptz_service ";
char *httpHeaderStatic = "HTTP/1.1\r\nUser-Agent: Arduino/1.0\r\nAccept: */*\r\nContent-Type: application/soap+xml; charset=utf-8;\r\nContent-Length: ";
char *soap_EnvelopeOpen = "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">";
char *soap_EnvelopeClose = "</s:Envelope>";
const char* contentType = "application/soap+xml; charset=utf-8";

struct soap_Header soap_Header =  {
    .soap_HeaderOpen = "<s:Header>",
    .soap_HeaderSecurity = "",
    .soap_HeaderClose = "</s:Header>"
};

struct soap_Body soap_Body =  {
    .soap_BodyOpen = "<s:Body>",
//    .soap_BodyOpen = "<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">",
    .soap_Command = NULL,
    .soap_BodyClose = "</s:Body>"
};

int getSoapHeaderLength(struct soap_Header *soap_Header) {
  return strlen(soap_Header->soap_HeaderOpen) + strlen(soap_Header->soap_HeaderSecurity) + strlen(soap_Header->soap_HeaderClose);
}

int getSoapBodyLength(struct soap_Body *soap_Body) {
  return strlen(soap_Body->soap_BodyOpen) + strlen(soap_Body->soap_Command) + strlen(soap_Body->soap_BodyClose);
}

char *Serialize_Header (struct soap_Header *soap_Header) {
  static char *soap_HeaderWhole;
  soap_HeaderWhole = (char*)malloc(2); //dirty hack, cant get it working properly
  strcpy(soap_HeaderWhole, soap_Header->soap_HeaderOpen);
  strcat(soap_HeaderWhole, soap_Header->soap_HeaderSecurity);
  strcat(soap_HeaderWhole, soap_Header->soap_HeaderClose);
  return soap_HeaderWhole;
}


char *Serialize_Body (struct soap_Body *soap_Body) {
  static char *soap_BodyWhole;
  soap_BodyWhole = (char*)malloc(2); //dirty hack, cant get it working properly
  strcpy(soap_BodyWhole, soap_Body->soap_BodyOpen);
  strcat(soap_BodyWhole, soap_Body->soap_Command);
  strcat(soap_BodyWhole, soap_Body->soap_BodyClose);
  return soap_BodyWhole;
}

 void printHash(uint8_t *hash) {
     int i;
     for (i = 0; i < 20; i++) {
         Serial.print("0123456789abcdef"[hash[i] >> 4]);
         Serial.print("0123456789abcdef"[hash[i] & 0xf]);
     }
     Serial.println();
 }

 void printNonce(byte nonce[]) {
     for (int i = 0; i < 22; i++) {
         Serial.print(nonce[i], HEX);
     }
     Serial.println();
 }

byte *getNonce() {
    static byte random_arr[22];
    randomSeed(analogRead(A0));
    for (int i = 0; i < 22; i++) {
        random_arr[i] = random(255);
        Serial.print(random_arr[i], HEX);
    }
    Serial.println();
    return random_arr;
}

uint8_t *getDigest(byte nonce_t[], char *password, char *created) {
//  Digest = B64ENCODE( SHA1( B64DECODE( Nonce ) + Date + Password ) )
  printNonce(nonce_t);
  char chars[22];
  char *conc;
  memcpy(chars, nonce_t, 22);
  chars[22] = '\0';
  Sha1.init();
  conc = (char*)malloc(strlen(chars) + strlen(password) + strlen(created));
  strcpy(conc, chars);
  strcat(conc, created);
  strcat(conc, password);
  Sha1.print(conc);
  return Sha1.result();
}

char *calculateHeaderSecurity(char *username, char *password, char *createTime, byte nonce_l[]) {
  static char* securityHeader;
  unsigned char *base64_digest;
  unsigned char *base64_nonce;

  static uint8_t *hash;

  char *onvif_security1 = "<Security s:mustUnderstand=\"1\" xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\"><UsernameToken><Username>";
  char *onvif_security2 = "</Username><Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">";
  char *onvif_security3 = "</Password><Nonce EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">";
  char *onvif_security4 = "</Nonce><Created xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">";
  char *onvif_security5 = "</Created></UsernameToken></Security>";

  base64_digest = (unsigned char*)malloc(encode_base64_length(22));
  base64_nonce = (unsigned char*)malloc(encode_base64_length(20));
  hash = getDigest(nonce_l, password, createTime);  
  encode_base64(hash, 20, base64_digest);
  encode_base64(nonce_l, 22, base64_nonce);

  securityHeader = (char*)malloc(strlen(onvif_security1) + strlen(username) + strlen(onvif_security2) + strlen(base64_digest) + strlen(onvif_security3) + strlen(base64_nonce) + strlen(onvif_security4) + strlen(createTime) + strlen(onvif_security5));
  strcpy(securityHeader, onvif_security1);
  strcat(securityHeader, username);
  strcat(securityHeader, onvif_security2);
  strcat(securityHeader, base64_digest);
  strcat(securityHeader, onvif_security3);
  strcat(securityHeader, base64_nonce);
  strcat(securityHeader, onvif_security4);
  strcat(securityHeader, createTime);
  strcat(securityHeader, onvif_security5);
  Serial.print(securityHeader);
  return securityHeader;
}

void onvifGetHostname(IPAddress server){
  char* contentType = "application/soap+xml; charset=utf-8";
  char* postData = "<s:Envelope><s:Body><ns0:GetHostname xmlns:ns0=\"http://www.onvif.org/ver10/device/wsdl\"/></s:Body></s:Envelope>";
  int port = 80;
  // Initialize the Ethernet client library
  // with the IP address and port of the server
  // that you want to connect to (port 80 is default for HTTP):
  EthernetClient eth;
  HttpClient client = HttpClient(eth, server, port);
  client.post("/onvif/device_service", contentType, postData);
  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

void onvifGetSystemDateAndTime(IPAddress server) {
  const char* postData = "<s:Envelope><s:Body><ns0:GetSystemDateAndTime xmlns:ns0=\"http://www.onvif.org/ver10/device/wsdl\"/></s:Body></s:Envelope>";
  int port = 80;
  // Initialize the Ethernet client library
  // with the IP address and port of the server
  // that you want to connect to (port 80 is default for HTTP):
  EthernetClient eth;
  HttpClient client = HttpClient(eth, server, port);
  client.post("/onvif/device_service", contentType, postData);
  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

void onvifContinuousMove(IPAddress server, struct soap_Header *soap_Header, struct soap_Body *soap_Body, char *ProfileToken, char* velocity_x, char* velocity_y) {
  char postData[1050];
  char* contentType = "application/soap+xml; charset=utf-8";
  int port = 80;
  // Initialize the Ethernet client library
  // with the IP address and port of the server
  // that you want to connect to (port 80 is default for HTTP):
  EthernetClient eth;
  Serial.println(soap_Header->soap_HeaderSecurity);
//  char* soapHeader = "<s:Header><Security s:mustUnderstand=\"1\" xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\"><UsernameToken><Username>admin</Username><Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">K2IcvAJ0Mq/4Rv9jYyrKBsgiJ1c=</Password><Nonce EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">TEbs4ad+2UWRiWKbPRxRyboEAAAAAA==</Nonce><Created xmlns=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">2022-02-11T22:32:40.593Z</Created></UsernameToken></Security></s:Header>";
  sprintf(postData, "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Header>%s</s:Header>%s<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>%s</ProfileToken><Velocity><PanTilt x=\"%s\" y=\"%s\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>%s%s", \
          soap_Header->soap_HeaderSecurity, soap_Body->soap_BodyOpen, ProfileToken, velocity_x, velocity_y, soap_Body->soap_BodyClose, soap_EnvelopeClose);  
  Serial.println(postData);
  HttpClient client = HttpClient(eth, server, port);
  client.post("/onvif/ptz_service", contentType, postData);
  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  Serial.println("Done sending");
}

char* getISOFormattedTime(NTPClient* timeClient) {
  // AVR implementation of localtime() uses midn. Jan 1 2000 as epoch 
  // so UNIX_OFFSET has to be applied to time returned by getEpochTime()
  // More info here: https://www.nongnu.org/avr-libc/user-manual/group__avr__time.html
  // https://forum.arduino.cc/index.php?topic=567637.0
  time_t rawtime = timeClient->getEpochTime() - UNIX_OFFSET;
  struct tm * ti;
  static char buf[] = "0000-00-00T00:00:00Z";
  ti = localtime (&rawtime);
  Serial.println(timeClient->getEpochTime());
  if (strftime(buf, sizeof buf, "%FT%TZ", ti)) {
    // Serial.print("Succsessfully formatted: ");
    // Serial.println(buf);
  } else {
    Serial.println("strftime failed");
  }
  return buf;
}
