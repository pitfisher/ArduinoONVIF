#include "Arduino.h"
#include "onvif.h"
#include "sha1.h"
#include "base64.hpp"

char *onvif_command_GetCapabilities = "<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetSystemDateAndTime = "<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetNetworkInterfaces = "<GetNetworkInterfaces xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>";
char *onvif_command_GetProfiles = "<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>";
char *onvif_command_GetServices = "<GetServices xmlns=\"http://www.onvif.org/ver10/media/wsdl\"><IncludeCapability>false</IncludeCapability></GetServices>";
char *onvif_command_ContinuousMove = "<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>Profile_1</ProfileToken><Velocity><PanTilt x=\"0\" y=\"0\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>";
char *httpHeaderStatic = "POST /onvif/PTZ HTTP/1.1\r\nUser-Agent: Arduino/1.0\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
char *soap_EnvelopeOpen = "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">";
char *soap_EnvelopeClose = "</s:Envelope>";

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

uint8_t *getDigest(byte nonce_t[], char *password, char *created) {
  char chars[20];
  char *conc;
  memcpy(chars, nonce_t, 20);
  chars[20] = '\0';
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

  base64_digest = (unsigned char*)malloc(encode_base64_length(20));
  base64_nonce = (unsigned char*)malloc(encode_base64_length(20));
  hash = getDigest(nonce_l, password, createTime);

  encode_base64(hash, 20, base64_digest);
  encode_base64(nonce_l, 20, base64_nonce);

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
  return securityHeader;
}

void onvifContinuousMove(IPAddress server, struct soap_Header *soap_Header, struct soap_Body *soap_Body, char *ProfileToken, char* velocity_x, char* velocity_y) {
  Serial.println(millis());	
  EthernetClient client;
  // soap_Header.soap_HeaderSecurity = (char*)malloc(2); //not sure how it works again; attempt to allocate proper amount of memory makes arduino hang
  // strcpy(soap_Header.soap_HeaderSecurity, calculateHeaderSecurity(username, password, createTime, nonce));

  // soap_Body->soap_Command = (char *)malloc(onvif_command_GetSystemDateAndTime);
  // strcpy(soap_Body->soap_Command, onvif_command_GetSystemDateAndTime);
  //Serial.print("connecting to ");
  //Serial.print(server);
  //Serial.println("...");
  //Serial.println(micros());	
  if (client.connect(server, 80)) {
    //Serial.print("connected to ");
    //Serial.println(client.remoteIP());
    //Serial.println(micros());	
    // Make a HTTP request:
    client.print(httpHeaderStatic);
    client.println(strlen(soap_EnvelopeOpen) + getSoapHeaderLength(soap_Header) + strlen(soap_Body->soap_BodyOpen)+ strlen("<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken></ProfileToken><Velocity><PanTilt x=\"\" y=\"\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>") + strlen(ProfileToken) + strlen(velocity_x) + strlen(velocity_y) + strlen(soap_Body->soap_BodyClose) + strlen(soap_EnvelopeClose)); //calculate and send Content-Length of request
    client.println();
    Serial.println(millis());	
    client.print(soap_EnvelopeOpen);
    client.print(Serialize_Header(soap_Header));
    // client.print(soap_Header->soap_HeaderOpen);
    // client.print(soap_Header->soap_HeaderSecurity);
    // client.print(soap_Header->soap_HeaderClose);
    Serial.println(millis());
    // client.print(//Serialize_Body(soap_Body));
    client.print(soap_Body->soap_BodyOpen);
    // client.print(onvif_command_GetSystemDateAndTime);
    // client.print("<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>Profile_1</ProfileToken><Velocity><PanTilt x=\"0.5\" y=\"0\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>");
    client.print("<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\"><ProfileToken>");
    client.print(ProfileToken);
    client.print("</ProfileToken><Velocity><PanTilt x=\"");
    client.print(velocity_x);
    client.print("\" y=\"");
    client.print(velocity_y);
    client.print("\" xmlns=\"http://www.onvif.org/ver10/schema\"/></Velocity></ContinuousMove>");
    client.print(soap_Body->soap_BodyClose);
    client.print(soap_EnvelopeClose);
    Serial.println(millis());	
    Serial.println("Done sending");
    // int len = client.available();
    //   while (len == 0) len = client.available();
    //   while (len > 0) {
    //     byte buffer[80];
    //     if (len > 80) len = 80;
    //     client.read(buffer, len);
    //     // if (printWebData) {
    //       Serial.write(buffer, len); // show in the //Serial monitor (slows some boards)
    //     // }
    //     // byteCount = byteCount + len;
    //     len = client.available();
    //   }
    client.stop();
    Serial.println(millis());
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
    delay(5);
  }
  //Serial.println("Done move");
  // free(soap_Body.soap_Command);
  Serial.println(millis());
}
