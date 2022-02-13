#ifndef ONVIF_H_INCLUDED
#define ONVIF_H_INCLUDED

#include <NTPClient.h>

char extern *onvif_command_GetCapabilities;
char extern *onvif_command_GetSystemDateAndTime;
char extern *onvif_command_GetNetworkInterfaces;
char extern *onvif_command_GetProfiles;
char extern *onvif_command_GetServices;
char extern *onvif_command_ContinuousMove;
char extern *httpHeaderStatic;
char extern *soap_EnvelopeOpen;
char extern *soap_EnvelopeClose;

struct soap_Header {
  char *soap_HeaderOpen;
  char *soap_HeaderSecurity;
  char *soap_HeaderClose;
};

struct soap_Body {
  char *soap_BodyOpen;
  char *soap_Command;
  char *soap_BodyClose;
};

struct soap_Envelope {
  struct soap_Header *soap_Header;
  struct soap_Body *soap_Body;
};

int getSoapHeaderLength(struct soap_Header *soap_Header);

int getSoapBodyLength(struct soap_Body *soap_Body);

char *serialize_Header (struct soap_Header *soap_Header);

char *serialize_Body (struct soap_Body *soap_Body);

uint8_t *getDigest(byte nonce_t[], char *password, char *created);

char *calculateHeaderSecurity(char *username, char *password, char *createTime, byte nonce_l[]);

//this function must be rewritten to exclude usage of Ethernet client
void onvifGetHostname(IPAddress server);
void onvifContinuousMove(IPAddress server, struct soap_Header *soap_Header, struct soap_Body *soap_Body, char *ProfileToken, char *velocity_x, char *velocity_y);
void onvifGetSystemDateAndTime(IPAddress server, struct soap_Header *soap_Header, struct soap_Body *soap_Body);

char* getISOFormattedTime(NTPClient* timeClient);

// char* compose_SOAP (struct soap_Header *soap_Header, struct soap_Body *soap_Body) {
// 	// if(authorization_needed) {
// 	soap_Header.soap_HeaderSecurity = (char*)malloc(2); //not sure how it works again; attempt to allocate proper amount of memory makes arduino hang
//     strcpy(soap_Header.soap_HeaderSecurity,calculateHeaderSecurity(username, password, createTime, nonce));

//     // allocating memory for ONVIF command text and putting it in Body structre
//     // probably should pack all commands in some kind of dictionary
//     soap_Body.soap_Command = (char *)malloc(onvif_command_GetSystemDateAndTime);
//   	strcpy(soap_Body.soap_Command,onvif_command_GetSystemDateAndTime);
// }

#endif // _ONVIF_H
