#pragma once
#ifndef TUBATECH_HOST_TEST
#include <Preferences.h>
#include <NetworkServer.h>
#include <NetworkClient.h>
#include <lwip/sockets.h>
#include <cerrno>
#include "wifi_config.h" // Local ignored file; tools/configure_wifi.py creates it.

namespace local {
inline NetworkServer server(80);
inline NetworkClient client;
inline HttpFrame frame;
inline std::string response;
inline size_t written=0;
inline uint32_t clientAt=0;
inline std::string apPassword;
inline bool saveSlave4(const char *mac){Preferences prefs;if(!prefs.begin("tubatech-v2",false))return false;bool ok=prefs.putString("slave4_mac",mac)>0;prefs.end();return ok;}
inline std::string randomKey(unsigned words){std::string s;char buf[9];for(unsigned i=0;i<words;++i){snprintf(buf,sizeof(buf),"%08lx",(unsigned long)esp_random());s+=buf;}return s;}
inline void openPair(bool admin){
  std::string code=randomKey(1);access.openPair(code,admin?Role::Admin:Role::Operator,millis());
  Serial.printf("PAIR %s code %s (120 seconds, one use, five attempts). Enter only in your phone app.\n",admin?"ADMIN":"OPERATOR",code.c_str());
}
inline bool beginWifi(){
  Preferences prefs;if(!prefs.begin("tubatech-v2",false))return false;
  std::string storedMac=prefs.getString("slave4_mac","").c_str();uint8_t mac[6];
  if(storedMac=="38:3E:51:D4:6A:30"&&nodecfg::parseMac(storedMac,mac)&&nodecfg::validSlave4(mac)){memcpy(nodecfg::positions[3].mac,mac,6);nodecfg::positions[3].installed=true;}
  // Configure at build time; normal operation never requires Serial setup.
  apPassword=TUBATECH_AP_PASSWORD;
  if(apPassword.size()<8||apPassword.size()>63)return false;
  access.operatorToken=prefs.getString("operator_token","").c_str();access.adminToken=prefs.getString("admin_token","").c_str();
  if(access.operatorToken.size()!=64){access.operatorToken=randomKey(8);prefs.putString("operator_token",access.operatorToken.c_str());}
  if(access.adminToken.size()!=64){access.adminToken=randomKey(8);prefs.putString("admin_token",access.adminToken.c_str());}prefs.end();
  if(!WiFi.softAP("TubaTech_Master",apPassword.c_str(),CHANNEL,false,2))return false;
  Serial.println("Local Wi-Fi: TubaTech_Master | API http://192.168.4.1 | channel 6");
  server.begin();server.setNoDelay(true);return true;
}
inline void closeClient(){client.stop();frame=HttpFrame{};response.clear();written=0;}
inline void pollWifi(){
  if(!client){client=server.accept();if(!client)return;client.setTimeout(0);clientAt=millis();frame=HttpFrame{};response.clear();written=0;}
  // Entire request and response have a bounded lifetime. Slow clients cannot
  // starve heartbeats; each loop consumes/writes at most 512 bytes.
  if(uint32_t(millis()-clientAt)>3000){closeClient();return;}
  if(response.empty()){
    for(unsigned i=0;i<512&&client.available()&&!frame.complete&&!frame.failed;++i)frame.feed(char(client.read()));
    if(frame.failed){closeClient();return;}
    if(!frame.complete)return;
    const auto remote=client.remoteIP();
    const auto ap=WiFi.softAPIP();
    frame.request.apClient=client.localIP()==ap&&remote[0]==ap[0]&&
      remote[1]==ap[1]&&remote[2]==ap[2]&&remote[3]!=0&&remote[3]!=255&&remote!=ap;
    Reply r=handle(frame.request);
    response="HTTP/1.1 "+std::to_string(r.code)+" Response\r\nContent-Type: application/json\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+std::to_string(r.body.size())+"\r\n\r\n"+r.body;
  }
  // NetworkClient inherits Print::availableForWrite() == 0 in core 3.3.12;
  // its write() also retries. Use one nonblocking socket send per tick instead.
  size_t n=response.size()-written;if(n>512)n=512;
  int sent=::send(client.fd(),response.data()+written,n,MSG_DONTWAIT);
  if(sent>0)written+=size_t(sent);
  else if(sent<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK){closeClient();return;}
  if(written==response.size())closeClient();
}
} // namespace local
#endif
