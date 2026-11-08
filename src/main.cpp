// Cardputer Suite P1: original stock M5Stack Cardputer K132.
// No Grove modules or Cardputer ADV hardware features are required.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include "core/Calculator.h"
#include "core/Paths.h"
#include "core/TextDocument.h"
#include "WifiLogger.h"
#include "MidiApp.h"
#include "PocketApps.h"

namespace {
constexpr uint16_t BG=0x0841, PANEL=0x10C4, SELECTED=0x2673;
constexpr uint16_t WHITE=0xFFFF, MUTED=0xAD55, CYAN=0x05FF, GREENISH=0x5FE9;
constexpr int SD_CS=12, SD_SCK=40, SD_MISO=39, SD_MOSI=14;
constexpr int EDIT_COLS=38, EDIT_ROWS=9, MAX_FILES=128;
constexpr char NOTE_PATH[]="/SUITE/NOTES.TXT";
constexpr char CONFIG_PATH[]="/SUITE/CONFIG.TXT";
constexpr uint8_t BRIGHTNESS[]={64,128,192,255};

enum class Screen : uint8_t { Home, Notes, Calculator, Files, Preview, Wifi, WifiLog, WifiLogView, WifiLogDetail, WifiLogErase, Midi, System, Settings, Stopwatch, Countdown, Units, Programmer };
const char *const APPS[]={"Notes","Calculator","Files","Wi-Fi Scan","Wi-Fi Logger","MIDI Studio","System Info","Settings","Stopwatch","Countdown","Unit Converter","Programmer"};
constexpr int APP_COUNT=12;
Screen screen=Screen::Home;
int appSelected=0, settingsSelected=0;
bool sdReady=false, noteWritable=true, dirty=true, scanActive=false, autosave=true;
uint8_t brightnessIndex=2;
String notice, expression, answer;
suite::TextDocument note, preview;
char browsePath[suite::paths::MaxPath]="/SUITE";
int filesSelected=0, filesCount=0;
String previewPath;
String ssid[7];
int rssi[7]={}, wifiCount=0;
uint32_t lastRefresh=0, lastWifiLogDraw=0, wifiLogViewSkip=0;
int wifiLogSelected=0;
bool hasViewedRecord=false;
suite::wifi_csv::Record viewedRecord;
WifiLogger wifiLogger;
MidiApp midiApp;
uint32_t lastMidiRefresh=0, lastPocketRefresh=0;
PocketApps pocketApps;

bool has(const Keyboard_Class::KeysState &k,char c) {
    for(char value:k.word) if(value==c) return true;
    return false;
}
bool hasLetter(const Keyboard_Class::KeysState &k,char lowercase) {
    return has(k,lowercase) || has(k,static_cast<char>(lowercase-'a'+'A'));
}
void writeText(int x,int y,const String &s,uint16_t color=WHITE) {
    auto &d=M5Cardputer.Display;
    d.setTextColor(color,BG);
    d.setCursor(x,y); d.print(s);
}
void frame(const char *caption,const String &hint) {
    auto &d=M5Cardputer.Display;
    d.fillScreen(BG);
    d.fillRect(0,0,240,23,PANEL);
    d.setTextColor(WHITE,PANEL); d.setCursor(5,7);d.print(caption);
    d.setTextColor(CYAN,PANEL);d.setCursor(207,7);d.print("P1");
    d.fillRect(0,116,240,19,PANEL);
    d.setTextColor(MUTED,PANEL);d.setCursor(4,121);d.print(hint.substring(0,39));
}

// Transaction-like SD replacement. Keep last good version at .BAK.
// After a reset between renames, recoverMissing() restores the last version.
void recoverMissing(const char *path) {
    if(!sdReady || SD.exists(path)) return;
    String backup=String(path)+".BAK";
    if(SD.exists(backup)) SD.rename(backup.c_str(),path);
}
bool writeSafely(const char *path,const char *data,size_t len) {
    if(!sdReady || !path || !data) return false;
    const String temp=String(path)+".NEW";
    const String backup=String(path)+".BAK";
    if(SD.exists(temp.c_str()) && !SD.remove(temp.c_str())) return false;
    File out=SD.open(temp.c_str(),FILE_WRITE);
    if(!out) return false;
    const size_t written=out.write(reinterpret_cast<const uint8_t*>(data),len);
    out.flush();
    const bool whole=(written==len && out.size()==len);
    out.close();
    if(!whole) return false; // original remains untouched
    const bool hadOld=SD.exists(path);
    if(hadOld) {
        if(SD.exists(backup.c_str()) && !SD.remove(backup.c_str())) return false;
        if(!SD.rename(path,backup.c_str())) return false;
    }
    if(!SD.rename(temp.c_str(),path)) {
        if(hadOld) SD.rename(backup.c_str(),path);
        return false;
    }
    return true;
}
bool loadText(const char *path,suite::TextDocument &destination) {
    File input=SD.open(path,FILE_READ);
    if(!input || input.isDirectory()) { if(input) input.close();return false; }
    if(input.size()>suite::TextDocument::Capacity) { input.close();return false; }
    static char buffer[suite::TextDocument::Capacity+1];
    const size_t len=input.size();
    const size_t got=input.readBytes(buffer,len);
    input.close();
    if(got!=len) return false;
    return destination.load(buffer,len);
}
bool saveNote() {
    if(!sdReady) { notice="No SD: text only in RAM";return false; }
    if(!noteWritable) { notice="Existing note unsafe; no overwrite";return false; }
    if(!writeSafely(NOTE_PATH,note.data(),note.length())) {
        notice="Save failed; check SD and backup";return false;
    }
    note.markSaved();notice="Saved NOTES.TXT";return true;
}
void saveConfig() {
    char config[32];
    const int n=snprintf(config,sizeof(config),"P1 %u %u\n",
                         static_cast<unsigned>(brightnessIndex),autosave?1u:0u);
    if(n<=0 || static_cast<size_t>(n)>=sizeof(config)) return;
    if(sdReady && !writeSafely(CONFIG_PATH,config,static_cast<size_t>(n)))
        notice="Settings save failed";
}
void applyBrightness() { M5Cardputer.Display.setBrightness(BRIGHTNESS[brightnessIndex]); }
void startStorage() {
    SPI.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
    sdReady=SD.begin(SD_CS,SPI,10000000);
    if(!sdReady) { notice="No SD: changes not persistent";return; }
    SD.mkdir("/SUITE");
    recoverMissing(NOTE_PATH);recoverMissing(CONFIG_PATH);
    if(SD.exists(NOTE_PATH) && !loadText(NOTE_PATH,note)) {
        noteWritable=false;
        notice="NOTES.TXT too big or not ASCII";
    }
    if(SD.exists(CONFIG_PATH)) {
        File f=SD.open(CONFIG_PATH,FILE_READ);
        if(f) {
            if(f.size()<32) {
                char config[32]={0};
                const size_t len=f.size();
                if(f.readBytes(config,len)==len) {
                    unsigned brightness=0, automatic=0;
                    if(sscanf(config,"P1 %u %u",&brightness,&automatic)==2 &&
                       brightness<4 && automatic<=1) {
                        brightnessIndex=brightness;
                        autosave=(automatic==1);
                    }
                }
            }
            f.close();
        }
    }
}
void drawHome() {
    frame("CARDPUTER SUITE",sdReady?"Enter open | SD ready":"Enter open | No SD card");
    auto &d=M5Cardputer.Display;
    const int top=appSelected>=7?appSelected-6:0;
    for(int i=top;i<APP_COUNT && i<top+7;++i) {
        const int y=25+(i-top)*12;
        if(appSelected==i) d.fillRoundRect(3,y-1,232,12,3,SELECTED);
        d.setTextColor(WHITE,appSelected==i?SELECTED:BG);
        d.setCursor(9,y+1);d.print(i+1);d.print("  ");d.print(APPS[i]);
    }
}
void drawDocument(const suite::TextDocument &doc,bool showCursor) {
    size_t caretRow=0,caretCol=0;
    doc.visualPosition(doc.cursor(),EDIT_COLS,caretRow,caretCol);
    const size_t top=caretRow>=EDIT_ROWS-1?caretRow-(EDIT_ROWS-2):0;
    char rows[EDIT_ROWS][EDIT_COLS+1]={};
    size_t row=0,col=0;
    const char *text=doc.data();
    for(size_t i=0;i<doc.length();++i) {
        if(text[i]=='\n') { ++row;col=0;continue; }
        if(row>=top && row<top+EDIT_ROWS) rows[row-top][col]=text[i];
        if(++col==EDIT_COLS) { ++row;col=0; }
    }
    for(int r=0;r<EDIT_ROWS;++r)
        writeText(5,26+r*10,String(rows[r]));
    if(showCursor && caretRow>=top && caretRow<top+EDIT_ROWS) {
        M5Cardputer.Display.drawRect(5+static_cast<int>(caretCol)*6,
            26+static_cast<int>(caretRow-top)*10,6,9,GREENISH);
    }
}
void drawNotes() {
    const String hint=notice.length()?notice:
        (noteWritable?(sdReady?"Ctrl+S save | arrows edit":"RAM only | arrows edit"):
                      "Protected: cannot overwrite file");
    frame(note.changed()?"NOTES *":"NOTES",hint);
    drawDocument(note,true);
}
void drawCalculator() {
    frame("CALCULATOR","Enter solve | Del erase");
    writeText(6,33,"Expression:",MUTED);
    writeText(6,51,expression.substring(0,38));
    writeText(6,77,answer.substring(0,38),GREENISH);
}
int countFiles() {
    if(!sdReady) return 0;
    File dir=SD.open(browsePath,FILE_READ);
    if(!dir || !dir.isDirectory()) {if(dir) dir.close();return 0;}
    int count=0;
    while(count<MAX_FILES) {
        File entry=dir.openNextFile();
        if(!entry) break;
        ++count;entry.close();
    }
    dir.close();return count;
}
void refreshFiles() {
    filesCount=countFiles();
    if(filesSelected>=filesCount) filesSelected=filesCount?filesCount-1:0;
    if(filesSelected<0) filesSelected=0;
}
bool selectedFile(char *path,size_t capacity,bool &directory) {
    if(!sdReady || filesSelected>=filesCount) return false;
    File dir=SD.open(browsePath,FILE_READ);
    if(!dir || !dir.isDirectory()) {if(dir) dir.close();return false;}
    bool success=false;
    for(int i=0;i<=filesSelected;++i) {
        File entry=dir.openNextFile();
        if(!entry) break;
        if(i==filesSelected) {
            const char *base=suite::paths::basename(entry.name());
            success=suite::paths::join(browsePath,base,path,capacity);
            directory=entry.isDirectory();
        }
        entry.close();
    }
    dir.close();return success;
}
void drawFiles() {
    frame("FILES",sdReady?"Enter open | Backspace up":"Insert FAT32 microSD");
    if(!sdReady) {writeText(5,37,"No card mounted",MUTED);return;}
    writeText(5,25,String(browsePath).substring(0,38),CYAN);
    File dir=SD.open(browsePath,FILE_READ);
    if(!dir || !dir.isDirectory()) {
        if(dir) dir.close();
        writeText(5,45,"Cannot open directory",MUTED);return;
    }
    const int top=filesSelected>=7?filesSelected-6:0;
    int idx=0;
    while(idx<filesCount) {
        File entry=dir.openNextFile();
        if(!entry) break;
        if(idx>=top && idx<top+7) {
            const int y=38+(idx-top)*11;
            if(idx==filesSelected) M5Cardputer.Display.fillRect(3,y-1,234,11,SELECTED);
            const char *base=suite::paths::basename(entry.name());
            const String line=String(entry.isDirectory()?"[D] ":"    ")+base;
            auto &d=M5Cardputer.Display;
            d.setTextColor(WHITE,idx==filesSelected?SELECTED:BG);
            d.setCursor(6,y);d.print(line.substring(0,37));
        }
        entry.close();++idx;
        if(idx>=top+7) break;
    }
    dir.close();
    if(filesCount==0) writeText(6,45,"(empty)",MUTED);
}
void drawPreview() {
    frame("TEXT VIEWER",String(suite::paths::basename(previewPath.c_str())).substring(0,38));
    drawDocument(preview,false);
}
void startScan() {
    wifiCount=0;scanActive=true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.scanDelete();
    if(WiFi.scanNetworks(true,false)==WIFI_SCAN_FAILED) {
        scanActive=false;notice="Wi-Fi scan failed";
    }
}
void stopScan() {
    WiFi.scanDelete();WiFi.mode(WIFI_OFF);scanActive=false;
}
void pollScan() {
    if(!scanActive) return;
    const int result=WiFi.scanComplete();
    if(result==WIFI_SCAN_RUNNING) return;
    scanActive=false;
    if(result>0) {
        wifiCount=result<7?result:7;
        for(int i=0;i<wifiCount;++i) {
            ssid[i]=WiFi.SSID(i);
            if(ssid[i].isEmpty()) ssid[i]="(hidden)";
            rssi[i]=WiFi.RSSI(i);
        }
    }
    WiFi.scanDelete();WiFi.mode(WIFI_OFF);dirty=true;
}
void drawWifi() {
    frame("WI-FI SCAN",notice.length()?notice:"R rescan | receive only");
    if(scanActive) {writeText(6,42,"Scanning 2.4 GHz networks...");return;}
    if(!wifiCount) {writeText(6,42,"No networks to display",MUTED);return;}
    for(int i=0;i<wifiCount;++i)
        writeText(5,28+i*12,(ssid[i].substring(0,26)+
             " "+String(rssi[i])+"dBm").substring(0,38));
}

String wifiName(const suite::wifi_csv::Record &r) {
    return r.ssid[0] ? String(r.ssid) : String("(hidden)");
}
String wifiSignal(int rssi) {
    // Indicative signal bars; not a calibrated distance measurement.
    const int bars=rssi>=-50?4:rssi>=-65?3:rssi>=-75?2:rssi>=-85?1:0;
    String label;
    for(int i=0;i<4;++i) label+=i<bars?'|':'.';
    return label;
}
void drawWifiLog() {
    frame("WI-FI LOGGER",wifiLogger.running ?
          "P pause  V saved  E erase  Enter info" :
          "S start  V saved  E erase  Enter info");
    if(!sdReady) {
        writeText(5,31,"Insert FAT32 microSD card",MUTED);
        writeText(5,48,"Recording disabled without SD");
        return;
    }
    writeText(5,25,(wifiLogger.running?"RUN ":"PAUSE ")+wifiLogger.status.substring(0,31),
              wifiLogger.running?GREENISH:MUTED);
    writeText(5,36,String("Scans ")+wifiLogger.scans+"  Hits "+wifiLogger.observations+
                    "  Now "+wifiLogger.lastSeen,CYAN);
    if(!wifiLogger.recentCount)
        writeText(5,55,"Press S to begin logging",MUTED);
    for(int i=0;i<wifiLogger.recentCount;++i) {
        const int y=48+i*11;
        if(i==wifiLogSelected)
            M5Cardputer.Display.fillRect(3,y-1,233,11,SELECTED);
        const auto &r=wifiLogger.recent[i];
        const String row=wifiName(r).substring(0,22)+" "+String(r.rssi)+
                         " "+wifiSignal(r.rssi);
        auto &d=M5Cardputer.Display;
        d.setTextColor(WHITE,i==wifiLogSelected?SELECTED:BG);
        d.setCursor(5,y);d.print(row.substring(0,38));
    }
}
void drawWifiLogRecord(const char *title,const suite::wifi_csv::Record &r,
                       const String &hint) {
    frame(title,hint);
    writeText(5,27,"SSID: "+wifiName(r).substring(0,30));
    writeText(5,39,String("BSSID: ")+r.bssid);
    writeText(5,51,String("RSSI: ")+r.rssi+" dBm  "+wifiSignal(r.rssi));
    writeText(5,63,String("Channel: ")+r.channel+" (2.4GHz)");
    writeText(5,75,String("Security: ")+r.security);
    writeText(5,87,String("Boot time: ")+r.uptimeMs+" ms");
    writeText(5,99,String("Session: ")+r.session+"  Scan "+r.scan);
}
void drawWifiLogView() {
    if(!hasViewedRecord) {
        frame("SAVED WI-FI LOG","B back  |  Start on logger");
        writeText(5,37,"No complete records found",MUTED);
        return;
    }
    drawWifiLogRecord("SAVED WI-FI LOG",viewedRecord,
                      "Up older / Down newer / B back");
    writeText(5,109,String("Previous records: ")+wifiLogViewSkip,MUTED);
}
void drawWifiLogDetail() {
    if(wifiLogger.recentCount==0) {
        frame("WI-FI DETAILS","B back");
        return;
    }
    drawWifiLogRecord("WI-FI DETAILS",wifiLogger.recent[wifiLogSelected],"B back");
}
void drawWifiLogErase() {
    frame("ERASE WI-FI LOG","Y delete  N cancel");
    writeText(6,37,"Delete WIFI.CSV from SD?",WHITE);
    writeText(6,55,"All recorded scans will be lost.",MUTED);
    writeText(6,77,"Y = ERASE",CYAN);
    writeText(6,90,"N = CANCEL",GREENISH);
}

void drawSystem() {
    frame("SYSTEM INFO","Original K132 / no PSRAM");
    writeText(6,28,"Board: Cardputer / StampS3");
    writeText(6,41,String("CPU: ")+ESP.getCpuFreqMHz()+" MHz");
    writeText(6,54,String("Flash: ")+ESP.getFlashChipSize()/1048576+" MB");
    writeText(6,67,String("Free heap: ")+ESP.getFreeHeap()/1024+" KB");
    writeText(6,80,String("Uptime: ")+millis()/1000+" s");
    writeText(6,93,sdReady?"microSD: mounted":"microSD: absent",sdReady?GREENISH:MUTED);
}
void drawSettings() {
    frame("SETTINGS",notice.length()?notice:"Arrows/Enter change | saves to SD");
    const String lines[2]={
        String("Brightness: ")+BRIGHTNESS[brightnessIndex]+"/255",
        String("Autosave notes: ")+(autosave?"ON":"OFF")
    };
    for(int i=0;i<2;++i) {
        const int y=36+i*23;
        if(i==settingsSelected) M5Cardputer.Display.fillRoundRect(3,y-2,233,17,3,SELECTED);
        M5Cardputer.Display.setTextColor(WHITE,i==settingsSelected?SELECTED:BG);
        M5Cardputer.Display.setCursor(8,y+2);
        M5Cardputer.Display.print(lines[i]);
    }
}
void render() {
    switch(screen) {
        case Screen::Home:drawHome();break;
        case Screen::Notes:drawNotes();break;
        case Screen::Calculator:drawCalculator();break;
        case Screen::Files:drawFiles();break;
        case Screen::Preview:drawPreview();break;
        case Screen::Wifi:drawWifi();break;
        case Screen::WifiLog:drawWifiLog();break;
        case Screen::WifiLogView:drawWifiLogView();break;
        case Screen::WifiLogDetail:drawWifiLogDetail();break;
        case Screen::WifiLogErase:drawWifiLogErase();break;
        case Screen::Midi:midiApp.render();break;
        case Screen::Stopwatch:case Screen::Countdown:
        case Screen::Units:case Screen::Programmer:
            pocketApps.draw(millis());break;
        case Screen::System:drawSystem();break;
        case Screen::Settings:drawSettings();break;
    }
    dirty=false;
}
void openApp(int index) {
    notice="";
    switch(index) {
        case 0:screen=Screen::Notes;break;
        case 1:screen=Screen::Calculator;break;
        case 2:screen=Screen::Files;refreshFiles();break;
        case 3:screen=Screen::Wifi;startScan();break;
        case 4:screen=Screen::WifiLog;break;
        case 5:screen=Screen::Midi;midiApp.enter(sdReady);break;
        case 6:screen=Screen::System;break;
        case 7:screen=Screen::Settings;break;
        case 8:screen=Screen::Stopwatch;pocketApps.enter(PocketApps::Mode::Stopwatch);break;
        case 9:screen=Screen::Countdown;pocketApps.enter(PocketApps::Mode::Countdown);break;
        case 10:screen=Screen::Units;pocketApps.enter(PocketApps::Mode::Units);break;
        case 11:screen=Screen::Programmer;pocketApps.enter(PocketApps::Mode::Programmer);break;
    }
    dirty=true;
}
void returnHome() {
    if(screen==Screen::Notes && note.changed() && autosave) saveNote();
    if(screen==Screen::Wifi) stopScan();
    if(screen==Screen::WifiLog || screen==Screen::WifiLogView ||
       screen==Screen::WifiLogDetail || screen==Screen::WifiLogErase)
        wifiLogger.pause();
    if(screen==Screen::Midi) midiApp.exit();
    screen=Screen::Home;dirty=true;
}
void fileEnter() {
    char name[suite::paths::MaxPath];
    bool directory=false;
    if(!selectedFile(name,sizeof(name),directory)) {notice="Unsupported file name";return;}
    if(directory) {
        strcpy(browsePath,name);filesSelected=0;refreshFiles();return;
    }
    if(!suite::paths::textFile(name)) {notice="Not a text file";return;}
    if(!loadText(name,preview)) {notice="Too large or binary (max 4 KB)";return;}
    previewPath=name;preview.toStart();screen=Screen::Preview;
}
void changeSetting(int direction) {
    if(settingsSelected==0) {
        brightnessIndex=static_cast<uint8_t>((brightnessIndex+4+direction)%4);
        applyBrightness();
    } else autosave=!autosave;
    notice="";saveConfig();
}
void input(const Keyboard_Class::KeysState &k) {
    // Physical top-left backtick returns to launcher. Fn + backtick = Escape.
    if(screen!=Screen::Home && (has(k,static_cast<char>(96)) || k.esc)) {
        returnHome();return;
    }
    switch(screen) {
        case Screen::Home:
            if(k.up || hasLetter(k,'w')) appSelected=(appSelected+APP_COUNT-1)%APP_COUNT;
            if(k.down || hasLetter(k,'s')) appSelected=(appSelected+1)%APP_COUNT;
            if(k.enter) openApp(appSelected);
            break;
        case Screen::Notes:
            notice="";
            if(k.ctrl && hasLetter(k,'s')) {saveNote();break;}
            if(k.left) note.left();
            if(k.right) note.right();
            if(k.up) note.up();
            if(k.down) note.down();
            if(k.backspace) note.backspace();
            if(k.del) note.eraseForward();
            if(k.enter) note.insert('\n');
            if(!k.ctrl && !k.fn && !k.alt) {
                for(char c:k.word) {
                    if(c==static_cast<char>(96)) continue;
                    if(!note.insert(c)) notice="4 KB document limit";
                }
            }
            break;
        case Screen::Calculator:
            if((k.backspace || k.del) && expression.length()) expression.remove(expression.length()-1);
            if(k.enter) {
                double value=0;
                answer=calc::evaluate(expression.c_str(),value)?"= "+String(value,5):
                                                                  "Syntax or math error";
            } else if(!k.ctrl && !k.fn && !k.alt) {
                for(char c:k.word) {
                    if(expression.length()>=48) break;
                    if((c>='0' && c<='9') || c=='.' || c=='+' || c=='-' ||
                       c=='*' || c=='/' || c=='(' || c==')' || c==' ')
                        expression+=c;
                }
            }
            break;
        case Screen::Files:
            if(k.up || hasLetter(k,'w')) {if(filesSelected>0) --filesSelected;}
            if(k.down || hasLetter(k,'s')) {if(filesSelected+1<filesCount) ++filesSelected;}
            if(k.backspace || k.del) {
                if(strcmp(browsePath,"/SUITE")!=0) {
                    char parent[suite::paths::MaxPath];
                    if(suite::paths::parent(browsePath,parent,sizeof(parent)) &&
                       strncmp(parent,"/SUITE",6)==0) {
                        strcpy(browsePath,parent);filesSelected=0;refreshFiles();
                    }
                }
            }
            if(k.enter) fileEnter();
            if(hasLetter(k,'r')) refreshFiles();
            break;
        case Screen::Preview:
            if(k.up) preview.up();
            if(k.down) preview.down();
            if(k.left) preview.left();
            if(k.right) preview.right();
            if(k.backspace) screen=Screen::Files;
            break;
        case Screen::Wifi:
            if(hasLetter(k,'r')) {stopScan();startScan();}
            break;
        case Screen::WifiLog:
            if(hasLetter(k,'s')) {wifiLogger.start(sdReady);break;}
            if(hasLetter(k,'p')) {wifiLogger.pause();break;}
            if(hasLetter(k,'e')) {
                wifiLogger.pause();screen=Screen::WifiLogErase;break;
            }
            if(hasLetter(k,'v')) {
                wifiLogger.pause();wifiLogViewSkip=0;
                hasViewedRecord=wifiLogger.readFromEnd(0,viewedRecord,sdReady);
                screen=Screen::WifiLogView;break;
            }
            if(k.up && wifiLogSelected>0) --wifiLogSelected;
            if(k.down && wifiLogSelected+1<wifiLogger.recentCount) ++wifiLogSelected;
            if(k.enter && wifiLogger.recentCount) {
                wifiLogger.pause();screen=Screen::WifiLogDetail;
            }
            break;
        case Screen::WifiLogView:
            if(k.backspace || hasLetter(k,'b')) {screen=Screen::WifiLog;break;}
            if(k.up && hasViewedRecord) {
                suite::wifi_csv::Record older;
                if(wifiLogger.readFromEnd(wifiLogViewSkip+1,older,sdReady)) {
                    ++wifiLogViewSkip;viewedRecord=older;
                }
            }
            if(k.down && wifiLogViewSkip>0) {
                suite::wifi_csv::Record newer;
                if(wifiLogger.readFromEnd(wifiLogViewSkip-1,newer,sdReady)) {
                    --wifiLogViewSkip;viewedRecord=newer;
                }
            }
            break;
        case Screen::WifiLogDetail:
            if(k.backspace || hasLetter(k,'b')) screen=Screen::WifiLog;
            break;
        case Screen::WifiLogErase:
            if(hasLetter(k,'y')) {
                wifiLogger.erase(sdReady);
                wifiLogSelected=0;screen=Screen::WifiLog;
            } else if(hasLetter(k,'n') || k.backspace) screen=Screen::WifiLog;
            break;
        case Screen::Midi:
            midiApp.onKey(k,sdReady);
            break;
        case Screen::Stopwatch:case Screen::Countdown:
        case Screen::Units:case Screen::Programmer:
            pocketApps.onKey(k,millis());break;
        case Screen::System:break;
        case Screen::Settings:
            if(k.up) settingsSelected=(settingsSelected+1)%2;
            if(k.down) settingsSelected=(settingsSelected+1)%2;
            if(k.left) changeSetting(-1);
            if(k.right || k.enter) changeSetting(1);
            break;
    }
    dirty=true;
}
} // namespace

void setup() {
    Serial.begin(115200);
    auto cfg=M5.config();
    M5Cardputer.begin(cfg,true);
    auto &d=M5Cardputer.Display;
    d.setRotation(1);
    d.setTextSize(1);
    d.setTextFont(1);
    d.setTextWrap(false);
    startStorage();
    applyBrightness();
    WiFi.mode(WIFI_OFF);
    dirty=true;render();
}
void loop() {
    M5Cardputer.update();
    if(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        const auto &keys=M5Cardputer.Keyboard.keysState();
        input(keys);
    }
    if(screen==Screen::Wifi) pollScan();
    if(screen==Screen::Midi) {
        midiApp.poll();
        if(millis()-lastMidiRefresh>=180) {
            lastMidiRefresh=millis();dirty=true;
        }
    }
    pocketApps.poll(millis());
    if(screen==Screen::Stopwatch || screen==Screen::Countdown ||
       screen==Screen::Units || screen==Screen::Programmer) {
        if(millis()-lastPocketRefresh>=120) {
            lastPocketRefresh=millis();dirty=true;
        }
    }
    if(wifiLogger.running) wifiLogger.tick();
    if(screen==Screen::WifiLog && millis()-lastWifiLogDraw>=750) {
        lastWifiLogDraw=millis();dirty=true;
        if(wifiLogSelected>=wifiLogger.recentCount)
            wifiLogSelected=wifiLogger.recentCount?wifiLogger.recentCount-1:0;
    }
    if(screen==Screen::System && millis()-lastRefresh>=1500) {
        lastRefresh=millis();dirty=true;
    }
    if(dirty) render();
    delay(5);
}
