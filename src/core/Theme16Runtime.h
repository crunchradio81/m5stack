#pragma once
// Shared original Cardputer K132 theme/profile persistence. SD only;
// settings never include passwords, tokens or network credentials.
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <M5Cardputer.h>
#include "Theme16Core.h"
namespace suite {namespace skin16 {
static constexpr const char*FilePath="/SUITE/CONFIG/PROFILES.CFG";
static constexpr const char*TempPath="/SUITE/CONFIG/PROFILES.NEW";
static constexpr const char*BackupPath="/SUITE/CONFIG/PROFILES.BAK";
struct Runtime {
 Settings settings;
 Palette colors;
 bool initialized=false,validFile=false,writable=true;
};
inline Runtime&state(){
 static Runtime runtime;return runtime;
}
inline const Palette&paletteRef(){return state().colors;}
inline bool validFile(){return state().validFile;}
inline bool writable(){return state().writable;}
inline const Settings&current(){return state().settings;}
inline void applyDisplay(){
 static const uint8_t brightness[]={64,128,192,255};
 const unsigned level=state().settings.active().brightness;
 M5Cardputer.Display.setBrightness(brightness[level<4?level:2]);
}
inline void adopt(const Settings&settings,bool isFile){
 Runtime&rt=state();
 rt.settings=settings;
 rt.colors=palette(settings.active().theme);
 rt.validFile=isFile;
 rt.initialized=true;
}
inline bool reload(){
 Runtime&rt=state();
 Settings defaults;
 rt.writable=true;
 if(!SD.exists(FilePath)){
  adopt(defaults,false);return true;
 }
 File file=SD.open(FilePath,FILE_READ);
 if(!file||file.isDirectory()||file.size()>Settings::MaxBytes){
  if(file)file.close();
  adopt(defaults,false);rt.writable=false;return false;
 }
 char data[Settings::MaxBytes+1]={};
 const size_t n=file.size();
 const bool complete=file.readBytes(data,n)==n;
 file.close();
 Settings incoming;
 if(!complete||!incoming.decode(data,n)){
  adopt(defaults,false);rt.writable=false;return false;
 }
 adopt(incoming,true);return true;
}
inline const Palette&colors(){
 if(!state().initialized)reload();
 return state().colors;
}
inline bool save(const Settings&candidate){
 Runtime&rt=state();
 if(!rt.initialized)reload();
 if(!rt.writable)return false;
 if(!SD.exists("/SUITE")&&!SD.mkdir("/SUITE"))return false;
 if(!SD.exists("/SUITE/CONFIG")&&!SD.mkdir("/SUITE/CONFIG"))
  return false;
 char buffer[Settings::MaxBytes+1]={};
 const size_t n=candidate.encode(buffer,sizeof(buffer));
 if(!n)return false;
 if(SD.exists(TempPath)&&!SD.remove(TempPath))return false;
 File f=SD.open(TempPath,FILE_WRITE);
 if(!f)return false;
 const bool success=f.write(reinterpret_cast<const uint8_t*>(buffer),n)==n;
 f.flush();
 const bool full=f.size()==n;
 f.close();
 if(!success||!full)return false;
 const bool old=SD.exists(FilePath);
 if(old){
  if(SD.exists(BackupPath)&&!SD.remove(BackupPath))return false;
  if(!SD.rename(FilePath,BackupPath))return false;
 }
 if(!SD.rename(TempPath,FilePath)){
  if(old)SD.rename(BackupPath,FilePath);
  return false;
 }
 adopt(candidate,true);
 applyDisplay();
 return true;
}
inline bool restoreBackup(){
 Runtime&rt=state();
 if(!rt.initialized)reload();
 // Only recover when active config is missing or invalid. Never discard
 // a verified newer primary settings file via a one-key shortcut.
 if(rt.validFile||!SD.exists(BackupPath))return false;
 File f=SD.open(BackupPath,FILE_READ);
 if(!f||f.isDirectory()||f.size()>Settings::MaxBytes){
  if(f)f.close();return false;
 }
 char buffer[Settings::MaxBytes+1]={};
 const size_t n=f.size();
 const bool complete=f.readBytes(buffer,n)==n;
 f.close();
 Settings recovered;
 if(!complete||!recovered.decode(buffer,n))return false;
 // Protect bad primary: rename it to .BAD instead of overwriting it.
 if(SD.exists(FilePath)){
  const char*bad="/SUITE/CONFIG/PROFILES.BAD";
  if(SD.exists(bad))return false;
  if(!SD.rename(FilePath,bad))return false;
 }
 if(!SD.rename(BackupPath,FilePath))return false;
 adopt(recovered,true);rt.writable=true;applyDisplay();return true;
}
}} // namespace suite::skin16
