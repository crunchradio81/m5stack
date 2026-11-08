#pragma once
#include "core/Theme16Runtime.h"
// Original stock Cardputer K132: SD-based type 0/1 MIDI player, approximate
// multi-voice GM family synth, no PSRAM, Grove, USB MIDI or external decoder.
#include <Arduino.h>
#include <SD.h>
#include <M5Cardputer.h>
#include <esp_timer.h>
#include <string.h>
#include <strings.h>
#include "midi/Smf.h"
#include "midi/Synth.h"
#include "midi/Seek25Core.h"
#include "core/Playlist21Core.h"

class MidiApp {
public:
    static constexpr const char* Folder="/SUITE/MIDI";
    static constexpr int MaxSongs=48;
    enum class Page:uint8_t {Library,Playing,Mixer,Help,Karaoke,Playlist};

    void enter(bool sdReady) {
        exit();
        volume_=suite::skin16::current().active().volume;
        page_=Page::Library;
        refresh(sdReady);
        loadPlaylist(sdReady);
    }
    void exit() {
        playing_=loaded_=hasNext_=endTail_=seeking_=false;
        lastLyric_="";
        speakerOff();source_.close();synth_.allSoundOff();
    }
    void poll() {
        if(seeking_){pollSeek();return;}
        if(!loaded_ || !playing_) return;
        const int64_t now=esp_timer_get_time();
        for(int i=0;i<96 && hasNext_ && due_<=now;++i) {
            const suite::smf::Event ev=next_;
            songMicros_=nextSongMicros_;
            dispatch(ev);lastTick_=ev.tick;
            if(!reader_.next(next_)) {
                if(reader_.failed()) {
                    status_="MIDI stream damaged";stop();return;
                }
                hasNext_=false;endTail_=true;tailDue_=now+350000;
                break;
            }
            schedule(next_);
        }
        if(endTail_ && now>=tailDue_) {
            if(loop_) restart();
            else if(autoNext_ && nextIndex()>=0) start(nextIndex());
            else {
                playing_=false;endTail_=false;status_="Finished";
                speakerOff();synth_.allSoundOff();
            }
            return;
        }
        if(!speakerOn_) return;
        // Speaker owns queued raw buffers. Keep 3 reusable buffers and no
        // more than 2 outstanding sound chunks on its virtual channel 0.
        for(int i=0;i<2;++i) {
            if(M5Cardputer.Speaker.isPlaying(0)>=2) break;
            int16_t* pcm=audio_[audioIndex_];
            synth_.render(pcm,AudioFrames);
            if(!M5Cardputer.Speaker.playRaw(pcm,AudioFrames,
                  suite::midi::Synth::SampleRate,false,1,0,false)) break;
            audioIndex_=(audioIndex_+1)%3;
        }
    }
    void onKey(const Keyboard_Class::KeysState& k,bool sdReady) {
        if(page_==Page::Library) {
            if(k.up || letter(k,'w')) {if(selected_>0)--selected_;}
            if(k.down || letter(k,'s')) {if(selected_+1<songCount_)++selected_;}
            if(letter(k,'r'))refresh(sdReady);
            if(letter(k,'q')){page_=Page::Playlist;return;}
            if(letter(k,'a')&&songCount_>0){
                if(queueReadOnly_){status_="Playlist protected / read-only";return;}
                if(queue_.add(songs_[selected_])) {
                    queueDirty_=true;
                    queueSelected_=static_cast<int>(queue_.count())-1;
                    status_="Added to queue: Q view, S save";
                }else status_="Already queued, invalid name or full";
                return;
            }
            if(k.enter && songCount_>0)start(selected_);
            return;
        }
        if(page_==Page::Playlist) {
            if(letter(k,'b')||k.backspace) {
                page_=loaded_?Page::Playing:Page::Library;return;
            }
            if(k.up && queueSelected_>0)--queueSelected_;
            if(k.down && queueSelected_+1<static_cast<int>(queue_.count()))++queueSelected_;
            if(letter(k,'s')){savePlaylist();return;}
            if(letter(k,'r')){
                if(queueDirty_)status_="Unsaved edits: S save before reload";
                else loadPlaylist(sdReady);
                return;
            }
            if(letter(k,'p')){
                queueActive_=!queueActive_;
                status_=queueActive_?"Playlist order ON":"Library order ON";
                return;
            }
            if(letter(k,'u')&&queueSelected_>0&&!queueReadOnly_ &&
               queue_.move(queueSelected_,-1)){
                --queueSelected_;queueDirty_=true;return;
            }
            if(letter(k,'j')&&queueSelected_+1<static_cast<int>(queue_.count()) &&
               !queueReadOnly_ && queue_.move(queueSelected_,1)){
                ++queueSelected_;queueDirty_=true;return;
            }
            if(letter(k,'d')&&!queueReadOnly_&&queue_.remove(queueSelected_)){
                if(queueSelected_>=static_cast<int>(queue_.count()) && queueSelected_>0)
                    --queueSelected_;
                queueDirty_=true;
                if(!queue_.count())queueActive_=false;
                return;
            }
            if(k.enter && queue_.count()){
                const int song=findSong(queue_.at(queueSelected_));
                if(song>=0){queueActive_=true;start(song);}
                else status_="Song absent: add file to /SUITE/MIDI";
                return;
            }
            return;
        }
        if(seeking_) {
            if(letter(k,'f'))seekBy(1);
            else if(letter(k,'j'))seekBy(-1);
            else if(letter(k,'p')||k.enter)seekResume_=!seekResume_;
            else if(letter(k,'b')||k.backspace){stop();page_=Page::Library;}
            return;
        }
        if(loaded_ && (page_==Page::Playing || page_==Page::Karaoke)) {
            if(letter(k,'f')){seekBy(1);return;}
            if(letter(k,'j')){seekBy(-1);return;}
        }
        if(page_==Page::Help) {
            if(k.backspace || letter(k,'b') || k.enter)page_=Page::Playing;
            return;
        }
        if(page_==Page::Karaoke) {
            if(k.backspace || letter(k,'b') || letter(k,'k')) {
                page_=Page::Playing;return;
            }
            if(k.enter || symbol(k,' ') || letter(k,'p'))toggle();
            return;
        }
        if(page_==Page::Mixer) {
            if(k.up && trackSelected_>0)--trackSelected_;
            if(k.down && trackSelected_+1<reader_.trackCount())++trackSelected_;
            if(letter(k,'s')) {
                solo_=(solo_==trackSelected_)?-1:trackSelected_;
                applyMute();
            }
            if(k.enter || letter(k,'m')) {
                manualMute_[trackSelected_]=!manualMute_[trackSelected_];
                applyMute();
            }
            if(letter(k,'b') || k.backspace)page_=Page::Playing;
            return;
        }
        if(k.enter || symbol(k,' ') || letter(k,'p')) {toggle();return;}
        if(letter(k,'b') || k.backspace) {
            exit();page_=Page::Library;return;
        }
        if(letter(k,'m')) {page_=Page::Mixer;return;}
        if(letter(k,'q')) {page_=Page::Playlist;return;}
        if(letter(k,'k')) {page_=Page::Karaoke;return;}
        if(letter(k,'h') || symbol(k,'?')) {page_=Page::Help;return;}
        if(letter(k,'l')) {loop_=!loop_;return;}
        if(letter(k,'a')) {autoNext_=!autoNext_;return;}
        if(symbol(k,',')) {changeTranspose(-1);return;}
        if(symbol(k,'.')) {changeTranspose(1);return;}
        if(letter(k,'z')) {restart();return;}
        if(letter(k,'n')) {
            const int next=nextIndex();
            if(next>=0)start(next);
            return;
        }
        if(symbol(k,'+') || symbol(k,'=')) {
            if(speed_<200)changeSpeed(speed_+10);
            return;
        }
        if(symbol(k,'-')) {
            if(speed_>50)changeSpeed(speed_-10);
            return;
        }
        if(symbol(k,']')) {
            if(volume_<160)volume_+=8;
            if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
            return;
        }
        if(symbol(k,'[')) {
            if(volume_>16)volume_-=8;
            if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
        }
    }
    void render() const {
        auto &d=M5Cardputer.Display;
        const auto &p=suite::skin16::colors();
        const uint16_t bg=p.bg,
        panel=p.panel,
        accent=p.accent,
        white=p.text,
        green=p.good,
        grey=p.muted,
        selected=p.selected;
        d.fillScreen(bg);d.fillRect(0,0,240,23,panel);
        d.setTextColor(white,panel);d.setCursor(5,7);
        d.print(page_==Page::Library?"MIDI LIBRARY":
                page_==Page::Playing?"MIDI STUDIO":
                page_==Page::Mixer?"MIDI MIXER":
                page_==Page::Karaoke?"MIDI KARAOKE":
                page_==Page::Playlist?"MIDI PLAYLIST":"MIDI CONTROLS");
        d.setTextColor(accent,panel);d.setCursor(212,7);d.print("P2");
        d.fillRect(0,116,240,19,panel);
        d.setTextColor(grey,panel);d.setCursor(4,121);
        switch(page_) {
            case Page::Library:d.print("Enter play A queue Q playlists");break;
            case Page::Playing:d.print("J/F seek -/+10s | M mix | K lyric");break;
            case Page::Mixer:d.print("Enter mute | S solo | B back");break;
            case Page::Help:d.print("B return to player");break;
            case Page::Karaoke:d.print("J/F seek | Enter pause | B back");break;
            case Page::Playlist:d.print("U/J move D del S save P order");break;
        }
        auto line=[&d,bg](int x,int y,const String& s,uint16_t color=0xffff) {
            d.setTextColor(color,bg);d.setCursor(x,y);d.print(s.substring(0,38));
        };
        if(page_==Page::Playlist) {
            line(5,25,String(queue_.count())+" queued / "+
                 (queueActive_?"play order ON":"library order")+
                 (queueDirty_?" *":""),accent);
            if(queueReadOnly_)line(5,37,"READ-ONLY: recover SD file",grey);
            const int top=queueSelected_>=6?queueSelected_-5:0;
            for(int i=top;i<static_cast<int>(queue_.count())&&i<top+6;++i){
                const int y=43+(i-top)*11;
                if(i==queueSelected_)d.fillRect(2,y-1,235,11,selected);
                d.setTextColor(white,i==queueSelected_?selected:bg);
                d.setCursor(5,y);
                d.print(String(i+1)+". "+String(queue_.at(i)).substring(0,28));
            }
            line(5,109,status_,grey);return;
        }
        if(page_==Page::Library) {
            line(5,26,String(songCount_)+" MIDI files in /SUITE/MIDI",accent);
            if(songCount_==0) {
                line(5,44,status_,grey);
                line(5,57,"Copy .MID or .KAR to SD",grey);
            } else {
                const int top=selected_>=7?selected_-6:0;
                for(int i=top;i<songCount_ && i<top+7;++i) {
                    const int y=37+(i-top)*11;
                    if(i==selected_) d.fillRect(2,y-1,235,11,selected);
                    d.setTextColor(white,i==selected_?selected:bg);
                    d.setCursor(5,y);d.print(String(i+1)+". "+songs_[i]);
                }
                line(5,105,queue_.count()?String("Queue ")+queue_.count()+
                     (queueDirty_?" unsaved":" saved"):status_,grey);
            }
            return;
        }
        if(page_==Page::Help) {
            line(5,27,"Enter/Space/P: Play or pause");
            line(5,39,"J/F: -/+10s seek  Z: Restart");
            line(5,51,"B: Library  N: Next  L: Loop");
            line(5,63,"[ / ] volume  , / . transpose");
            line(5,75,"A auto-next M mix K lyric Q queue");
            line(5,87,"Mixer: Fn arrows / Enter mute");
            line(5,101,"Built-in speaker, no SoundFont",grey);
            return;
        }
        if(!loaded_) {line(5,37,status_,grey);return;}
        if(seeking_){
            line(5,38,String("Seeking to ")+
                 static_cast<uint32_t>(seekScanner_.goal()/1000000ULL)+"s",accent);
            line(5,53,String("Checked ")+seekScanner_.scanned()+" events",grey);
            line(5,67,"Skipped notes stay silent",grey);
            line(5,85,"P/Enter: resume mode | B cancel",grey);
            return;
        }
        if(page_==Page::Karaoke) {
            line(5,26,songs_[selected_],accent);
            line(5,40,String(playing_?"PLAYING":"PAUSED")+
                       "  tick "+lastTick_,playing_?green:grey);
            line(5,56,"Synchronized MIDI lyric:",grey);
            d.setTextColor(white,bg);
            d.setTextSize(2);
            d.setCursor(5,73);
            d.print(lastLyric_.substring(0,19));
            d.setTextSize(1);
            if(lastLyric_.length()>19)
                line(5,102,lastLyric_.substring(19,52),white);
            return;
        }
        if(page_==Page::Mixer) {
            line(5,26,String("Tracks ")+reader_.trackCount()+
                     "  Solo "+(solo_<0?String("OFF"):String(solo_+1)),accent);
            const int top=trackSelected_>=7?trackSelected_-6:0;
            for(int i=top;i<reader_.trackCount() && i<top+7;++i) {
                const int y=37+(i-top)*11;
                if(i==trackSelected_)d.fillRect(2,y-1,235,11,selected);
                d.setTextColor(white,i==trackSelected_?selected:bg);
                d.setCursor(5,y);
                d.print(String("Track ")+(i+1)+
                    (synth_.trackMuted(i)?" [MUTE] ":" [ON] ")+
                    "v"+String(synth_.activeForTrack(i)));
            }
            return;
        }
        line(5,26,songs_[selected_],accent);
        line(5,39,String(playing_?"PLAY ":"PAUSE ")+
                  "Fmt"+reader_.format()+" / "+reader_.trackCount()+" tracks",
                  playing_?green:grey);
        const uint32_t bpm=tempo_?60000000u/tempo_:0;
        line(5,52,String("Tempo ")+bpm+" BPM  Speed "+speed_+"%");
        line(5,65,String("At ")+static_cast<uint32_t>(positionUs()/1000000ULL)+"s"+
                    "  Tick "+lastTick_+"  "+numerator_+"/"+(1u<<denominatorPower_));
        line(5,78,String("Voices ")+synth_.activeVoices()+
                    "/"+suite::midi::Synth::Voices+"  Vol "+volume_+
                    "  Loop "+(loop_?"Y":"N")+"  Next "+(autoNext_?"Y":"N")+
                    (queueActive_?" Q":""));
        line(5,91,String("Note ")+lastNote_+"  Prog "+lastProgram_+
                    "  Trans "+transpose_);
        // Live chromatic note activity: 12 pitch-class columns.
        for(int i=0;i<12;++i) {
            const size_t count=synth_.activeForPitchClass(i);
            const int height=static_cast<int>(count>4?12:count*3);
            d.drawRect(5+i*19,100,16,12,grey);
            if(height>0)d.fillRect(6+i*19,111-height,14,height,green);
        }
    }

private:
    class SdSource:public suite::smf::Source {
    public:
        bool open(const char* path) {
            close();file_=SD.open(path,FILE_READ);
            return file_ && !file_.isDirectory();
        }
        void close(){if(file_)file_.close();}
        uint32_t length() const override {
            return file_?static_cast<uint32_t>(file_.size()):0;
        }
        bool readAt(uint32_t offset,uint8_t* out,size_t n) override {
            return file_ && file_.seek(offset) &&
                   file_.read(out,n)==static_cast<int>(n);
        }
    private:
        File file_;
    };
    static constexpr size_t AudioFrames=256;
    char songs_[MaxSongs][64]={{0}};
    int songCount_=0,selected_=0,trackSelected_=0,solo_=-1;
    bool manualMute_[suite::smf::Reader::MaxTracks]={};
    bool loaded_=false,playing_=false,loop_=false,autoNext_=false,speakerOn_=false;
    bool hasNext_=false,endTail_=false,seeking_=false,seekResume_=false;
    suite::midi25::Scanner seekScanner_;
    uint64_t songMicros_=0,nextSongMicros_=0;
    suite::playlist21::Queue queue_;
    bool queueActive_=false,queueReadOnly_=true,queueDirty_=false;
    int queueSelected_=0;
    Page page_=Page::Library;
    int speed_=100,transpose_=0;
    uint8_t volume_=64;
    int64_t due_=0,pauseAt_=0,tailDue_=0;
    uint32_t lastTick_=0,tempo_=500000;
    uint8_t numerator_=4,denominatorPower_=2,lastProgram_=0;
    String lastNote_="--",lastLyric_,status_;
    int16_t audio_[3][AudioFrames]={};
    int audioIndex_=0;
    SdSource source_;
    suite::smf::Reader reader_;
    suite::smf::Event next_;
    suite::midi::Synth synth_;

    static bool symbol(const Keyboard_Class::KeysState& k,char c) {
        for(char x:k.word)if(x==c)return true;
        return false;
    }
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        return symbol(k,c)||symbol(k,static_cast<char>(c-'a'+'A'));
    }
    static bool isMidi(const char* n) {
        if(!n)return false;
        const char* dot=strrchr(n,'.');if(!dot)return false;
        char suffix[6]={};const size_t len=strlen(dot);
        if(len>=sizeof(suffix))return false;
        for(size_t i=0;i<len;++i) {
            char c=dot[i];suffix[i]=(c>='A'&&c<='Z')?static_cast<char>(c+32):c;
        }
        return strcmp(suffix,".mid")==0||strcmp(suffix,".midi")==0||
               strcmp(suffix,".kar")==0;
    }
    void refresh(bool sdReady) {
        songCount_=0;selected_=0;
        if(!sdReady){status_="No SD mounted";return;}
        if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
        if(!SD.exists(Folder)&&!SD.mkdir(Folder)) {
            status_="Cannot make /SUITE/MIDI";return;
        }
        File dir=SD.open(Folder,FILE_READ);
        if(!dir||!dir.isDirectory()) {
            if(dir)dir.close();status_="SD open failed";return;
        }
        while(songCount_<MaxSongs) {
            File f=dir.openNextFile();if(!f)break;
            String name=f.name();
            const int slash=name.lastIndexOf('/');
            if(slash>=0)name=name.substring(slash+1);
            if(!f.isDirectory() && isMidi(name.c_str()) &&
               name.length()<sizeof(songs_[0])) {
                strncpy(songs_[songCount_],name.c_str(),sizeof(songs_[0])-1);
                ++songCount_;
            }
            f.close();
        }
        dir.close();
        for(int i=0;i<songCount_;++i)
            for(int j=i+1;j<songCount_;++j)
                if(strcasecmp(songs_[i],songs_[j])>0) {
                    char tmp[64];strcpy(tmp,songs_[i]);
                    strcpy(songs_[i],songs_[j]);strcpy(songs_[j],tmp);
                }
        status_=songCount_?"Select a song":"No .MID/.KAR files found";
    }

    static constexpr const char* QueuePath="/SUITE/MIDI/PLAYLIST.PLS";
    int findSong(const char*name)const{
        if(!name)return -1;
        for(int i=0;i<songCount_;++i)
            if(suite::playlist21::same(songs_[i],name))return i;
        return -1;
    }
    int nextIndex()const{
        if(queueActive_){
            const unsigned n=queue_.count();
            if(!n)return -1;
            const int current=selected_<songCount_?
                queue_.indexOf(songs_[selected_]):-1;
            const unsigned first=current<0?0:(static_cast<unsigned>(current)+1)%n;
            for(unsigned i=0;i<n;++i){
                const int index=findSong(queue_.at((first+i)%n));
                if(index>=0)return index;
            }
            return -1;
        }
        return songCount_>1?(selected_+1)%songCount_:-1;
    }
    bool verifyPlaylistFile(const char*path,const char*expected,size_t expectedSize) {
        File f=SD.open(path,FILE_READ);
        if(!f || f.isDirectory() || f.size()!=expectedSize){
            if(f)f.close();return false;
        }
        char bytes[suite::playlist21::MaxBytes+1]={};
        const bool ok=f.readBytes(bytes,expectedSize)==expectedSize &&
                      memcmp(bytes,expected,expectedSize)==0;
        f.close();return ok;
    }
    void loadPlaylist(bool sdReady) {
        queue_.clear();queueSelected_=0;queueDirty_=false;
        queueActive_=false;queueReadOnly_=true;
        if(!sdReady){status_="Insert FAT32 SD for playlists";return;}
        if(SD.exists("/SUITE/MIDI/PLAYLIST.NEW")){
            status_="Interrupted playlist .NEW; recover manually";return;
        }
        if(!SD.exists(QueuePath)){
            if(SD.exists("/SUITE/MIDI/PLAYLIST.BAK"))
                status_="Backup-only playlist: recover manually";
            else {queueReadOnly_=false;status_="Playlist empty. A adds songs.";}
            return;
        }
        File file=SD.open(QueuePath,FILE_READ);
        if(!file||file.isDirectory()||file.size()>suite::playlist21::MaxBytes){
            if(file)file.close();status_="Playlist invalid: protected";return;
        }
        char data[suite::playlist21::MaxBytes+1]={};
        const size_t len=file.size();
        const bool ok=file.readBytes(data,len)==len;file.close();
        if(!ok || !queue_.parse(data,len)){
            status_="Playlist damaged: protected";return;
        }
        queueReadOnly_=false;
        status_=String("Playlist loaded: ")+queue_.count()+" songs";
    }
    bool savePlaylist(){
        if(queueReadOnly_){status_="Playlist protected; no overwrite";return false;}
        if(!queueDirty_){status_="Playlist already saved";return true;}
        char data[suite::playlist21::MaxBytes+1]={};
        const size_t len=queue_.encode(data,sizeof(data));
        if(!len){status_="Playlist encode failed";return false;}
        const char*temp="/SUITE/MIDI/PLAYLIST.NEW";
        const char*backup="/SUITE/MIDI/PLAYLIST.BAK";
        if(SD.exists(temp)){status_="Protected interrupted .NEW";return false;}
        File output=SD.open(temp,FILE_WRITE);
        if(!output){status_="Cannot create playlist .NEW";return false;}
        const bool ok=output.write(reinterpret_cast<const uint8_t*>(data),len)==len;
        output.flush();output.close();
        if(!ok||!verifyPlaylistFile(temp,data,len)){
            status_="Playlist temp verify failed";return false;
        }
        const bool existed=SD.exists(QueuePath);
        if(!existed&&SD.exists(backup)){
            status_="Backup-only state protected";return false;
        }
        if(existed){
            // Parsed old playlist and verified new temp. Rotate .BAK now.
            if(SD.exists(backup)&&!SD.remove(backup)){
                status_="Cannot rotate playlist backup";return false;
            }
            if(!SD.rename(QueuePath,backup)){
                status_="Cannot back up existing playlist";return false;
            }
        }
        if(!SD.rename(temp,QueuePath)){
            if(existed)SD.rename(backup,QueuePath);
            status_="Playlist commit failed";return false;
        }
        if(!verifyPlaylistFile(QueuePath,data,len)){
            status_="Playlist final verify failed: keep backup";
            queueReadOnly_=true;return false;
        }
        queueDirty_=false;
        status_=String("Saved ")+queue_.count()+" songs; prior .BAK";
        return true;
    }

    void speakerOff() {
        if(speakerOn_)M5Cardputer.Speaker.stop(0);
        speakerOn_=false;
    }
    void start(int index) {
        if(index<0||index>=songCount_)return;
        selected_=index;loaded_=playing_=seeking_=false;
        char path[96];
        const int len=snprintf(path,sizeof(path),"%s/%s",Folder,songs_[index]);
        if(len<0||static_cast<size_t>(len)>=sizeof(path)) {
            status_="Filename too long";return;
        }
        speakerOff();source_.close();
        if(!source_.open(path)||!reader_.open(source_)) {
            source_.close();status_="Unsupported/damaged MIDI";return;
        }
        synth_.reset();memset(manualMute_,0,sizeof(manualMute_));
        solo_=-1;trackSelected_=0;
        tempo_=500000;numerator_=4;denominatorPower_=2;
        lastTick_=0;songMicros_=nextSongMicros_=0;
        lastNote_="--";lastProgram_=0;lastLyric_="";
        hasNext_=reader_.next(next_);
        if(reader_.failed()||!hasNext_) {
            source_.close();status_="Empty or invalid MIDI";return;
        }
        due_=esp_timer_get_time();schedule(next_);
        speakerOn_=M5Cardputer.Speaker.begin();
        if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
        playing_=loaded_=true;page_=Page::Playing;
        endTail_=false;audioIndex_=0;
        status_=speakerOn_?"Wave synth (approx GM)":"Speaker unavailable";
    }
    void restart() {
        if(!loaded_)return;
        const int oldSpeed=speed_;
        const uint8_t oldVolume=volume_;
        const bool oldLoop=loop_;
        start(selected_);
        speed_=oldSpeed;volume_=oldVolume;loop_=oldLoop;
        if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
    }
    void toggle() {
        if(!loaded_)return;
        if(playing_) {
            playing_=false;pauseAt_=esp_timer_get_time();
            speakerOff();status_="Paused";return;
        }
        const int64_t now=esp_timer_get_time();
        const int64_t pausedFor=now-pauseAt_;
        due_+=pausedFor;if(endTail_)tailDue_+=pausedFor;
        speakerOn_=M5Cardputer.Speaker.begin();
        if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
        audioIndex_=0;playing_=true;status_="Playing";
    }
    void stop() {
        playing_=loaded_=hasNext_=endTail_=seeking_=false;
        speakerOff();source_.close();synth_.allSoundOff();
    }
    uint64_t positionUs()const {
        if(seeking_)return seekScanner_.goal();
        if(!hasNext_)return songMicros_;
        const int64_t now=playing_?esp_timer_get_time():pauseAt_;
        return suite::midi25::playbackPosition(songMicros_,nextSongMicros_,
                                               due_,now,speed_);
    }
    void seekBy(int direction) {
        if(!loaded_)return;
        const uint64_t origin=seeking_?seekScanner_.goal():positionUs();
        const uint64_t destination=suite::midi25::target(origin,direction);
        if(destination==origin)return;
        const bool resume=seeking_?seekResume_:playing_;
        speakerOff();synth_.allSoundOff();
        playing_=false;endTail_=false;seeking_=false;
        if(!reader_.open(source_)) {
            stop();status_="Seek: MIDI read failed";return;
        }
        hasNext_=reader_.next(next_);
        if(reader_.failed()||!hasNext_){
            stop();status_="Seek: invalid/empty MIDI";return;
        }
        synth_.reset();seekScanner_.begin(destination);
        seekResume_=resume;seeking_=true;
        lastTick_=0;tempo_=500000;numerator_=4;denominatorPower_=2;
        lastProgram_=0;lastNote_="--";lastLyric_="";
        songMicros_=nextSongMicros_=0;
        status_="Seeking (skipped notes silent)";
    }
    void pollSeek(){
        // Cooperative scan: bounded event count per UI frame, no audio.
        for(unsigned i=0;i<suite::midi25::EventsPerPoll;++i){
            uint64_t at=0;
            const auto result=seekScanner_.inspect(next_,reader_.division(),at);
            if(result==suite::midi25::Scanner::Result::Invalid){
                stop();status_="Seek limit or damaged MIDI";return;
            }
            if(result==suite::midi25::Scanner::Result::Ready){
                const int64_t delay=suite::midi25::delayAtSpeed(
                    at-seekScanner_.goal(),static_cast<unsigned>(speed_));
                if(delay<0){stop();status_="Seek timing out of range";return;}
                const int64_t now=esp_timer_get_time();
                lastTick_=seekScanner_.tick();
                songMicros_=seekScanner_.position();nextSongMicros_=at;
                tempo_=seekScanner_.tempo();due_=now+delay;
                pauseAt_=now;seeking_=false;endTail_=false;
                applyMute();
                playing_=seekResume_;
                if(playing_){
                    speakerOn_=M5Cardputer.Speaker.begin();
                    if(speakerOn_)M5Cardputer.Speaker.setVolume(volume_);
                }
                audioIndex_=0;
                status_=playing_?"Seek complete: playing":"Seek complete: paused";
                return;
            }
            const suite::smf::Event ev=next_;
            if(ev.kind==suite::smf::Kind::Tempo)tempo_=ev.tempo;
            else if(ev.kind==suite::smf::Kind::TimeSignature){
                numerator_=ev.numerator;
                denominatorPower_=ev.denominatorPower>6?6:ev.denominatorPower;
            }else if(ev.kind==suite::smf::Kind::Lyric)lastLyric_=String(ev.text);
            else if(ev.kind==suite::smf::Kind::Program)lastProgram_=ev.a;
            // Restore synth state without replaying skipped note attacks.
            if(ev.kind==suite::smf::Kind::Program ||
               ev.kind==suite::smf::Kind::Control ||
               ev.kind==suite::smf::Kind::PitchBend)
                synth_.event(ev);
            hasNext_=reader_.next(next_);
            if(reader_.failed()){
                stop();status_="Seek: corrupted MIDI stream";return;
            }
            if(!hasNext_){
                songMicros_=seekScanner_.position();
                nextSongMicros_=songMicros_;lastTick_=seekScanner_.tick();
                seeking_=playing_=endTail_=false;
                pauseAt_=esp_timer_get_time();
                status_="Reached end of MIDI song";return;
            }
        }
    }
    void changeSpeed(int newSpeed) {
        if(newSpeed<50 || newSpeed>200 || newSpeed==speed_)return;
        // Rescale the remaining delay to the next event immediately.
        const int64_t reference=playing_?esp_timer_get_time():pauseAt_;
        if(hasNext_ && due_>reference)
            due_=reference+(due_-reference)*speed_/newSpeed;
        speed_=newSpeed;
    }
    void changeTranspose(int delta) {
        const int target=transpose_+delta;
        if(target< -12 || target>12)return;
        transpose_=target;
        // Avoid orphan notes when a key is transposed while held.
        synth_.allSoundOff();
    }
    void schedule(const suite::smf::Event& ev) {
        nextSongMicros_=suite::midi25::eventTime(songMicros_,lastTick_,
                                      ev.tick,tempo_,reader_.division());
        const uint32_t ticks=ev.tick-lastTick_;
        const uint64_t micro=static_cast<uint64_t>(ticks)*tempo_*100u/
                             (static_cast<uint64_t>(reader_.division())*speed_);
        due_+=static_cast<int64_t>(micro);
    }
    void applyMute() {
        for(uint8_t i=0;i<reader_.trackCount();++i)
            synth_.muteTrack(i,manualMute_[i]||(solo_>=0&&i!=solo_));
    }
    void dispatch(const suite::smf::Event& ev) {
        switch(ev.kind) {
            case suite::smf::Kind::Tempo:tempo_=ev.tempo;break;
            case suite::smf::Kind::TimeSignature:
                numerator_=ev.numerator;
                denominatorPower_=ev.denominatorPower>6?6:ev.denominatorPower;
                break;
            case suite::smf::Kind::NoteOn:lastNote_=String(ev.a);break;
            case suite::smf::Kind::Program:lastProgram_=ev.a;break;
            case suite::smf::Kind::Lyric:
                lastLyric_=String(ev.text);break;
            default:break;
        }
        if(ev.kind==suite::smf::Kind::NoteOn ||
           ev.kind==suite::smf::Kind::NoteOff) {
            suite::smf::Event shifted=ev;
            int pitch=static_cast<int>(ev.a)+transpose_;
            if(pitch<0)pitch=0;
            if(pitch>127)pitch=127;
            shifted.a=static_cast<uint8_t>(pitch);
            synth_.event(shifted);
        } else synth_.event(ev);
    }
};
