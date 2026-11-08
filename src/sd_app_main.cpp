// Stock Cardputer K132 SD app firmware entrypoint. Installed only in OTA_0
// by the suite factory launcher. The original board has no PSRAM.
#if defined(SUITE_APP_METRONOME) || defined(SUITE_APP_PIANO) || \
    defined(SUITE_APP_CHANNELS) || defined(SUITE_APP_BLE) || \
    defined(SUITE_APP_ARCADE) || defined(SUITE_APP_OFFICE) || \
    defined(SUITE_APP_INSPECT) || defined(SUITE_APP_MUSICLAB) || \
    defined(SUITE_APP_DEVTOOLS) || defined(SUITE_APP_SYSTEM)
#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#if defined(SUITE_APP_DEVTOOLS)
#include "DevCollection.h"
using ActiveApp=DevCollection;
#elif defined(SUITE_APP_SYSTEM)
#include "SystemCollection.h"
using ActiveApp=SystemCollection;
#elif defined(SUITE_APP_ARCADE)
#include "ArcadeCollection.h"
using ActiveApp=ArcadeCollection;
#elif defined(SUITE_APP_OFFICE)
#include "OfficeCollection.h"
using ActiveApp=OfficeCollection;
#elif defined(SUITE_APP_INSPECT)
#include "InspectCollection.h"
using ActiveApp=InspectCollection;
#elif defined(SUITE_APP_MUSICLAB)
#include "MusicCollection.h"
using ActiveApp=MusicCollection;
#else
#include "StandaloneApps.h"
using ActiveApp=StandaloneApp;
#endif

namespace {
ActiveApp app;
bool returnPrepared=false;
uint32_t drawnAt=0;
bool escapeKey(const Keyboard_Class::KeysState& k) {
    if(k.esc)return true;
    for(char c:k.word)if(c==static_cast<char>(96))return true;
    return false;
}
bool prepareReturn() {
    const esp_partition_t *running=esp_ota_get_running_partition();
    const esp_partition_t *factory=esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_FACTORY,nullptr);
    if(!running||!factory||running->subtype!=ESP_PARTITION_SUBTYPE_APP_OTA_0 ||
       running->address==factory->address) return false;
    return esp_ota_set_boot_partition(factory)==ESP_OK;
}
}
void setup() {
    Serial.begin(115200);
    // Set NEXT boot back to factory before starting any app drivers.
    returnPrepared=prepareReturn();
    auto cfg=M5.config();
    M5Cardputer.begin(cfg,true);
    auto& d=M5Cardputer.Display;
    d.setRotation(1);d.setTextSize(1);d.setTextFont(1);d.setTextWrap(false);
    if(!returnPrepared) {
        d.fillScreen(0x0841);d.setTextColor(0xffff,0x0841);
        d.setCursor(6,35);d.print("BOOT PARTITION ERROR");
        d.setCursor(6,49);d.print("USB recovery may be needed.");
        Serial.println("SD app: failed to arm factory return");
        return;
    }
    app.begin();
    app.render();
}
void loop() {
    if(!returnPrepared){delay(100);return;}
    M5Cardputer.update();
    if(M5Cardputer.Keyboard.isChange() &&
       M5Cardputer.Keyboard.isPressed()) {
        auto k=M5Cardputer.Keyboard.keysState();
        if(escapeKey(k)) {
            app.exit();
            ESP.restart();
        }
        app.input(k);
    }
    app.poll();
    if(app.dirty() || millis()-drawnAt>=150) {
        drawnAt=millis();
        app.render();
    }
    delay(4);
}
#endif
