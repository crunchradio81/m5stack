#pragma once
#include "core/Theme16Runtime.h"
// P8 Networking collection for ORIGINAL K132, 2.4GHz Wi-Fi, no PSRAM.
// Owner controls all connections; SD file holds plaintext Wi-Fi password.
// HTTP/MQTT/LAN chat use plaintext transport; do not send secrets over them.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <time.h>
#include <PubSubClient.h>
#include "ping/ping_sock.h"
#include "lwip/ip_addr.h"
#include "core/Net8Core.h"

class NetworkCollection {
public:
  void begin(){
    menu_=-1;chosen_=0;dirty_=true;configured_=false;
    SPI.begin(40,39,14,12);
    sd_=SD.begin(12,SPI,10000000);
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    status_=sd_?"N = load config, S = connect":"Insert FAT32 SD";
    display_="Network OFF";
    loadConfig();
    mqtt_.setClient(wifiClient_);
    mqtt_.setCallback(mqttCallback);
    mqtt_.setSocketTimeout(2);
    mqtt_.setKeepAlive(20);
    active()=this;
  }
  void exit(){
    stopMode();
    if(ping_){esp_ping_stop(ping_);/* final callback is asynchronous */ }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    if(active()==this)active()=nullptr;
  }
  bool dirty()const{return dirty_;}
  void poll(){
    if(connecting_){
      if(WiFi.status()==WL_CONNECTED){
        connecting_=false;
        status_="Connected to 2.4GHz Wi-Fi";
        display_=String("IP ")+WiFi.localIP().toString();
        dirty_=true;
      }else if(static_cast<uint32_t>(millis()-connectAt_)>=12000){
        connecting_=false;
        WiFi.disconnect();
        status_="Wi-Fi failed / timed out";
        dirty_=true;
      }
    }
    if(pingDone_){
      pingDone_=false;
      if(ping_){esp_ping_delete_session(ping_);ping_=nullptr;}
      status_=pingOk_?String("ICMP Echo ")+pingMs_+" ms":"ICMP timeout/unreachable";
      dirty_=true;
    }
    if(menu_==6&&mqtt_.connected()) {
      mqtt_.loop();
      if(static_cast<uint32_t>(millis()-lastDraw_)>=800){
        lastDraw_=millis();dirty_=true;
      }
    }
    if(menu_==7&&chatStarted_){
      int sz=udp_.parsePacket();
      if(sz>0){
        uint8_t buf[112]={};
        if(sz<=108){
          const int n=udp_.read(buf,sizeof(buf));
          char from[22]={},message[73]={};
          if(n>0&&suite::net8::parseChat(buf,n,from,message)){
            display_=String(from)+": "+message;
            status_="Received local UDP chat";
            dirty_=true;
          }
        }else {while(udp_.available())udp_.read();}
      }
    }
  }
  void input(const Keyboard_Class::KeysState&k){
    if(menu_<0){
      if(k.up&&chosen_>0)--chosen_;
      if(k.down&&chosen_<8)++chosen_;
      if(k.enter)open(chosen_);
      dirty_=true;return;
    }
    if((menu_==7 && k.ctrl && letter(k,'b')) ||
       (menu_!=7 && letter(k,'b'))){
      stopMode();menu_=-1;dirty_=true;return;
    }
    if(menu_==7) {
      if(k.backspace){size_t n=strlen(typed_);if(n)typed_[n-1]=0;}
      else if(k.enter)sendChat();
      else if(!k.fn&&!k.ctrl&&!k.alt)appendInput(k);
      dirty_=true;return;
    }
    if(menu_==0){
      if(letter(k,'n'))loadConfig();
      if(letter(k,'s')||k.enter)connect();
      if(letter(k,'d')) {
        stopMode();WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
        connecting_=false;status_="Disconnected";display_="Network OFF";
      }
    } else if(letter(k,'s')||k.enter){
      run();
    }
    dirty_=true;
  }
  void render(){
    dirty_=false;
    auto&d=M5Cardputer.Display;
    const auto &p=suite::skin16::colors();
        const uint16_t bg=p.bg,
        head=p.panel,
        white=p.text,
        cyan=p.accent,
        green=p.good,
        sel=p.selected,
        dim=p.muted;
    d.fillScreen(bg);d.fillRect(0,0,240,23,head);
    d.setTextColor(white,head);d.setCursor(5,7);
    d.print(menu_<0?"NETWORK & INTERNET":label(menu_));
    d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
    d.fillRect(0,116,240,19,head);
    d.setTextColor(dim,head);d.setCursor(4,121);
    d.print(menu_<0?"Fn arrows / Enter":menu_==0?"N config S connect D disconnect":
             menu_==7?"Enter send | Ctrl+B menu":
             "S / Enter run | B menu");
    if(menu_<0){
      const int first=chosen_>=7?chosen_-6:0;
      for(int i=first;i<9&&i<first+7;++i){
        const int y=28+(i-first)*12;
        if(i==chosen_)d.fillRect(3,y-1,233,11,sel);
        d.setTextColor(white,i==chosen_?sel:bg);
        d.setCursor(8,y);d.print(String(i+1)+". "+label(i));
      }
      d.setTextColor(dim,bg);d.setCursor(5,109);
      d.print(configured_?"Config loaded from SD":"Needs NET.CFG on SD");
      return;
    }
    auto line=[&d,bg](int y,const String&s,uint16_t color){
      d.setTextColor(color,bg);d.setCursor(5,y);
      d.print(s.substring(0,38));
    };
    line(27,WiFi.status()==WL_CONNECTED?
      String("ONLINE ")+WiFi.localIP().toString():
      connecting_?"CONNECTING...":"OFFLINE",green);
    if(menu_==0) {
      line(43,configured_?String("SSID: ")+config_.ssid:"No valid Wi-Fi configuration",cyan);
      line(60,String("Signal: ")+WiFi.RSSI()+" dBm",white);
      line(75,"Credentials never printed on screen",dim);
    } else if(menu_==6){
      line(44,mqtt_.connected()?"MQTT CONNECTED":"MQTT OFFLINE",cyan);
      line(59,String("Topic: ")+config_.mqttTopic,white);
      line(75,"No TLS / local broker only",dim);
    } else if(menu_==7){
      line(44,"UDP broadcast port 38945",cyan);
      line(59,String("> ")+typed_,white);
      line(75,display_,green);
      line(96,"Visible to LAN. Not encrypted.",dim);
    } else if(menu_==8){
      line(43,"Basic RSS <title> extraction",cyan);
      line(59,"Uses configured HTTP endpoint",white);
      line(76,display_,green);
    } else {
      line(44,display_,cyan);
      if(menu_==1)line(61,String("DNS host: ")+config_.dnsHost,white);
      if(menu_==2)line(61,String("ICMP target: ")+config_.tcpHost,white);
      if(menu_==3)line(61,"Clock is UTC (NTP), not GPS",white);
      if(menu_==4)line(61,String("GET ")+config_.httpHost+config_.httpPath,white);
      if(menu_==5)line(61,"Discover _http._tcp services",white);
      if(modeLines_[0])line(78,modeLines_,white);
    }
    line(102,status_,dim);
  }
private:
  static const char*label(int i){
    static const char* names[]={"Wi-Fi Connection","DNS Lookup","ICMP Ping",
      "NTP UTC Clock","HTTP Text GET","LAN mDNS Discovery",
      "MQTT Subscriber","LAN UDP Chat","RSS Title Reader"};
    return names[i];
  }
  int menu_=-1,chosen_=0;
  bool sd_=false,configured_=false,connecting_=false,chatStarted_=false,
       dirty_=true,typing_=false,mdns_=false;
  uint32_t connectAt_=0,lastDraw_=0,lastChatSend_=0;
  suite::net8::Config config_;
  String status_,display_,modeLines_;
  char typed_[73]={},httpBuffer_[1025]={};
  WiFiClient wifiClient_;
  PubSubClient mqtt_;
  WiFiUDP udp_;
  esp_ping_handle_t ping_=nullptr;
  volatile bool pingDone_=false,pingOk_=false;
  volatile uint32_t pingMs_=0;
  static NetworkCollection*&active(){
    static NetworkCollection*instance=nullptr;return instance;
  }
  static void mqttCallback(char*topic,uint8_t*payload,unsigned int len){
    NetworkCollection* p=active();if(!p||p->menu_!=6)return;
    char value[81]={};
    const unsigned n=len<80?len:80;
    for(unsigned i=0;i<n;++i)
      value[i]=suite::net8::safeResponseChar(static_cast<char>(payload[i]))?
        static_cast<char>(payload[i]):'?';
    p->display_=String(topic).substring(0,20)+": "+value;
    p->status_=String("MQTT message (")+len+" bytes)";
    p->dirty_=true;
  }
  static void pingSuccess(esp_ping_handle_t h,void*arg){
    auto*p=reinterpret_cast<NetworkCollection*>(arg);
    if(!p)return;
    uint32_t ms=0;
    if(esp_ping_get_profile(h,ESP_PING_PROF_TIMEGAP,&ms,sizeof(ms))==ESP_OK)
      p->pingMs_=ms;
    p->pingOk_=true;
  }
  static void pingTimeout(esp_ping_handle_t,void*arg){
    auto*p=reinterpret_cast<NetworkCollection*>(arg);
    if(p)p->pingOk_=false;
  }
  static void pingEnd(esp_ping_handle_t,void*arg){
    auto*p=reinterpret_cast<NetworkCollection*>(arg);
    if(p)p->pingDone_=true;
  }
  static bool letter(const Keyboard_Class::KeysState&k,char c){
    for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
    return false;
  }
  void appendInput(const Keyboard_Class::KeysState&k){
    for(char c:k.word){
      if(c<32||c>126||c=='|')continue;
      const size_t n=strlen(typed_);
      if(n+1<sizeof(typed_)){typed_[n]=c;typed_[n+1]=0;}
    }
  }
  void loadConfig(){
    configured_=false;
    if(!sd_){status_="Insert SD card";return;}
    File f=SD.open("/SUITE/NET/NET.CFG",FILE_READ);
    if(!f||f.isDirectory()||f.size()>1024){
      if(f)f.close();status_="Missing/oversize NET.CFG";return;
    }
    char raw[1025]={};const size_t n=f.size();
    const bool read=f.readBytes(raw,n)==n;f.close();
    suite::net8::Config next;
    if(!read||!suite::net8::parse(raw,n,next)){
      status_="Invalid NET.CFG, no connection";return;
    }
    config_=next;configured_=true;status_="Profile loaded (password hidden)";
  }
  void connect(){
    if(!configured_){status_="Load valid NET.CFG first";return;}
    stopMode();
    WiFi.persistent(false);WiFi.mode(WIFI_STA);
    WiFi.begin(config_.ssid,config_.password);
    connecting_=true;connectAt_=millis();status_="Connecting up to 12s";
  }
  bool online(){
    if(WiFi.status()!=WL_CONNECTED){status_="Use Wi-Fi Connection first";return false;}
    return true;
  }
  void stopMode(){
    mqtt_.disconnect();
    if(chatStarted_){udp_.stop();chatStarted_=false;}
    if(mdns_){MDNS.end();mdns_=false;}
  }
  void open(int m){
    stopMode();
    menu_=m;dirty_=true;modeLines_="";display_="";
    status_="Press S / Enter to run";
    if(m==7){
      typed_[0]=0;
      if(online()&&udp_.begin(38945)){
        chatStarted_=true;status_="LAN chat ready";
      }else status_="Cannot start UDP chat";
    }
    if(m==0){
      status_=configured_?"S connect / D disconnect":"N load SD config";
      display_=WiFi.status()==WL_CONNECTED?"Connected":"Offline";
    }
  }
  void run(){
    if(!online())return;
    switch(menu_){
      case 1:dns();break;
      case 2:ping();break;
      case 3:ntp();break;
      case 4:http(false);break;
      case 5:mdns();break;
      case 6:mqtt();break;
      case 8:http(true);break;
      default:break;
    }
  }
  void dns(){
    IPAddress ip;
    const uint32_t at=millis();
    if(WiFi.hostByName(config_.dnsHost,ip)){
      display_=config_.dnsHost;modeLines_=ip.toString();
      status_=String("DNS resolved in ")+static_cast<uint32_t>(millis()-at)+" ms";
    }else status_="DNS lookup failed";
  }
  void ping(){
    if(ping_){status_="Ping in progress";return;}
    IPAddress ip;
    if(!WiFi.hostByName(config_.tcpHost,ip)){status_="Cannot resolve ICMP host";return;}
    esp_ping_config_t options=ESP_PING_DEFAULT_CONFIG();
    ip_addr_t target;
    IP_ADDR4(&target,ip[0],ip[1],ip[2],ip[3]);
    options.target_addr=target;
    options.count=1;options.timeout_ms=1500;options.interval_ms=1000;
    esp_ping_callbacks_t cb={};
    cb.on_ping_success=pingSuccess;
    cb.on_ping_timeout=pingTimeout;
    cb.on_ping_end=pingEnd;
    cb.cb_args=this;
    pingOk_=false;pingDone_=false;
    if(esp_ping_new_session(&options,&cb,&ping_)!=ESP_OK) {
      ping_=nullptr;status_="Cannot start ICMP session";return;
    }
    if(esp_ping_start(ping_)!=ESP_OK){
      esp_ping_delete_session(ping_);ping_=nullptr;
      status_="ICMP start error";return;
    }
    display_=String("Echo ")+ip.toString();status_="ICMP request sent";
  }
  void ntp(){
    configTime(0,0,"pool.ntp.org","time.google.com");
    struct tm utc={};
    if(getLocalTime(&utc,2200)){
      char when[40]={};
      strftime(when,sizeof(when),"%Y-%m-%d %H:%M:%S UTC",&utc);
      display_=when;status_="NTP-backed UTC reading";
    }else status_="NTP sync not ready; retry";
  }
  void http(bool rss){
    if(!suite::net8::host(config_.httpHost)){
      status_="Set HTTP_HOST/PORT/PATH in SD config";return;
    }
    // Deliberately limited to explicit plaintext HTTP; no TLS bypass.
    WiFiClient client;
    HTTPClient http;
    const String url=String("http://")+config_.httpHost+":"+
       config_.httpPort+config_.httpPath;
    if(!http.begin(client,url)){status_="HTTP begin failed";return;}
    http.setTimeout(2500);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    const int code=http.GET();
    if(code<200||code>=300){
      status_=String("HTTP status ")+code;
      http.end();return;
    }
    if(http.getSize()>2048){status_="HTTP response too large";http.end();return;}
    WiFiClient*stream=http.getStreamPtr();
    size_t n=0;const uint32_t started=millis();
    while(n<sizeof(httpBuffer_)-1 &&
          static_cast<uint32_t>(millis()-started)<2400 &&
          (http.connected()||stream->available())){
      if(stream->available()){
        const int c=stream->read();
        if(c>=0)httpBuffer_[n++]=static_cast<char>(c);
      }else delay(2);
    }
    httpBuffer_[n]=0;http.end();
    if(rss){
      char *open=strstr(httpBuffer_,"<title>");
      if(open){
        open+=7;
        char*close=strstr(open,"</title>");
        if(close){
          const size_t len=static_cast<size_t>(close-open);
          char title[81]={};
          suite::net8::safePrintLine(open,len,title,sizeof(title));
          display_=title;
          status_="RSS title (first 1 KiB only)";
          return;
        }
      }
      status_="No XML <title> within response limit";
    }else{
      char clean[161]={};
      suite::net8::safePrintLine(httpBuffer_,n,clean,sizeof(clean));
      display_=String(clean).substring(0,100);
      status_=String("HTTP ")+code+" / "+n+" bytes shown";
    }
  }
  void mdns(){
    if(!mdns_){
      if(!MDNS.begin("k132-net-tools")){status_="mDNS initialization failed";return;}
      mdns_=true;
    }
    const int found=MDNS.queryService("http","tcp");
    if(found>0){
      display_=String(MDNS.hostname(0))+":" +MDNS.port(0);
      modeLines_=MDNS.IP(0).toString();
      status_=String(found)+" local HTTP services discovered";
    }else status_="No _http._tcp services discovered";
  }
  void mqtt(){
    if(!suite::net8::host(config_.mqttHost)){
      status_="Configure LAN MQTT_HOST first";return;
    }
    mqtt_.setServer(config_.mqttHost,config_.mqttPort);
    const String id=String("k132-")+String(static_cast<uint32_t>(ESP.getEfuseMac()),HEX);
    if(mqtt_.connect(id.c_str())){
      if(mqtt_.subscribe(config_.mqttTopic)){
        status_="Subscribed to LAN broker (unencrypted)";
        display_="Awaiting messages";
      }else status_="Subscribe failed";
    }else status_=String("MQTT connect failed: ")+mqtt_.state();
  }
  void sendChat(){
    if(!chatStarted_||!online()){status_="LAN chat unavailable";return;}
    if(static_cast<uint32_t>(millis()-lastChatSend_)<800){status_="Send rate limited";return;}
    char packet[112]={};
    if(!suite::net8::makeChat(config_.chatName,typed_,packet,sizeof(packet))){
      status_="Chat 1–72 ASCII chars required";return;
    }
    const IPAddress local=WiFi.localIP(),mask=WiFi.subnetMask();
    IPAddress broadcast;
    for(int i=0;i<4;++i)broadcast[i]=local[i]|static_cast<uint8_t>(~mask[i]);
    if(udp_.beginPacket(broadcast,38945)){
      udp_.write(reinterpret_cast<const uint8_t*>(packet),strlen(packet));
      if(udp_.endPacket()){
        display_=String("Sent: ")+typed_;
        status_="LAN UDP broadcast; not encrypted";
        typed_[0]=0;lastChatSend_=millis();return;
      }
    }
    status_="Chat send failed";
  }
};
