// Bu dosyayı aynı klasörde "secrets.h" adıyla KOPYALAYIN ve kendi bilgilerinizi yazın.
// secrets.h .gitignore'dadır: GitHub'a gönderilmez. Bu örnek dosyaya gerçek şifre yazmayın.
#pragma once

// Wi-Fi (ESP32 yalnızca 2,4 GHz ağlara bağlanır)
#define WIFI_SSID "wifi-adiniz"
#define WIFI_PASS "wifi-sifreniz"

// MQTT sunucusu – Adafruit IO (TLS, 8883). Başka bir sunucu da kullanılabilir.
#define MQTT_HOST "io.adafruit.com"
#define MQTT_PORT 8883
#define MQTT_USER "adafruit-kullanici-adiniz"
#define MQTT_PASS "aio_xxxxxxxxxxxxxxxxxxxxxxxxxxxx"   // Adafruit IO Key (io.adafruit.com → sağ üstte anahtar simgesi)

// Konular (Adafruit IO'da "kullanici/feeds/feed-adi" biçimindedir)
#define TOPIC_TELE MQTT_USER "/feeds/h2-olcum"   // ESP32 → ikiz: ölçümler (JSON)
#define TOPIC_CMD  MQTT_USER "/feeds/h2-fan"     // ikiz → ESP32: fan komutu (auto | on | off | 0–100)
