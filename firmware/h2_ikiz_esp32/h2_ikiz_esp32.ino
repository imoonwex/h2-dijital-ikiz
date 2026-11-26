/*
  H₂ dijital ikiz – ESP32 ölçüm düğümü
  ------------------------------------
  Ölçer:  INA219 #1 (0x40) PV hattı      : Vin+ panel +, Vin− elektrolizör +
          INA219 #2 (0x41) yakıt hücresi : Vin+ yakıt hücresi +, Vin− yük +   (A0 köprüsü lehimli)
          2 × DS18B20 (tek 1-Wire hattı, 4,7 kΩ pull-up): yakıt hücresi ve elektrolizör sıcaklığı
  Gösterir: SSD1306 0,96" OLED (0x3C) – üst 16 piksel sarı (durum), alt kısım mavi (ölçümler)
  Sürer:  D4184 MOSFET modülüyle fan (PWM en çok %55: setin fanı 3 V, besleme 5 V)
  Gönderir: her 5 s'de bir JSON telemetri (MQTT, TLS) → dijital ikiz ve bulut
  Alır:   fan komutu: "auto" | "on" | "off" | 0–100 (yüzde, en çok FAN_MAX_PCT)

  Pinler: GPIO21 SDA, GPIO22 SCL, GPIO4 DS18B20, GPIO27 MOSFET PWM. Modüller 3V3'ten, bütün GND'ler ortak.

  Gerekli kütüphaneler (Arduino IDE → Kütüphane Yöneticisi):
    Adafruit INA219, Adafruit SSD1306, Adafruit GFX Library, OneWire, DallasTemperature,
    PubSubClient (Nick O'Leary), ArduinoJson (Benoit Blanchon, sürüm 7)
  Kart: "ESP32 Dev Module" (esp32 by Espressif, 2.x veya 3.x)

  ŞİFRELER: secrets.example.h dosyasını secrets.h adıyla kopyalayıp doldurun.
  secrets.h .gitignore'dadır; GitHub'a gönderilmez. Bu dosyaya hiçbir şifre yazmayın.
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// ---------------- pinler ----------------
#define PIN_SDA     21
#define PIN_SCL     22
#define PIN_ONEWIRE 4
#define PIN_FAN     27

// ---------------- ayarlar (şifre değil, kodda kalabilir) ----------------
const uint8_t  FAN_MAX_PCT   = 55;      // 5 V × %55 ≈ 2,75 V ortalama: setin fanı en çok 3 V
const float    FAN_ON_C      = 40.0;    // otomatik mod: bu sıcaklıkta fan açılır
const float    FAN_OFF_C     = 35.0;    // ... bu sıcaklığın altına inince kapanır (histerezis)
const float    T_WARN_C      = 55.0;    // bu sıcaklığın üstü uyarı
const float    I_WARN_A      = 1.90;    // INA219 en çok 2 A ölçer; bunun üstü uyarı
const uint32_t SENSOR_MS     = 1000;    // ölçüm aralığı
const uint32_t PUBLISH_MS    = 5000;    // yayın aralığı (Adafruit IO ücretsiz: dakikada en çok 30 mesaj)
const uint8_t  TEMP_FC_INDEX = 0;       // 1-Wire hattındaki hangi sensör yakıt hücresinde? (açılışta Seri Monitör'e yazılır)
const uint32_t FAN_PWM_HZ    = 20000;
const uint8_t  FAN_PWM_BITS  = 8;

// ESP32 Arduino çekirdeği 3.x ile 2.x arasında PWM işlevleri değişti
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  #define FAN_PWM_INIT()  Serial.printf("Fan PWM (GPIO%d, %lu Hz): %s\n", PIN_FAN, (unsigned long)FAN_PWM_HZ, ledcAttach(PIN_FAN, FAN_PWM_HZ, FAN_PWM_BITS) ? "hazir" : "BASARISIZ")
  #define FAN_PWM_WRITE(d) ledcWrite(PIN_FAN, (d))
#else
  #define FAN_PWM_INIT()  do { ledcSetup(0, FAN_PWM_HZ, FAN_PWM_BITS); ledcAttachPin(PIN_FAN, 0); } while (0)
  #define FAN_PWM_WRITE(d) ledcWrite(0, (d))
#endif

// ---------------- nesneler ----------------
Adafruit_INA219  inaPV(0x40), inaFC(0x41);
OneWire          oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
WiFiClientSecure net;
PubSubClient     mqtt(net);

struct Line { float v = NAN, vl = NAN, i = NAN, p = NAN; };   // v: kaynak ucu (Vin+), vl: yük ucu (Vin−)
Line pv, fc;
float tFC = NAN, tEL = NAN;
bool  okPV = false, okFC = false, okOled = false;
DeviceAddress addrT[2]; uint8_t nTemp = 0;

enum FanMode { FAN_AUTO, FAN_ON, FAN_OFF, FAN_PCT };
FanMode fanMode = FAN_AUTO;
uint8_t fanCmdPct = FAN_MAX_PCT, fanPct = 0;
bool    fanAutoOn = false;

String   warn[8]; uint8_t nWarn = 0;
uint32_t tSensor = 0, tPublish = 0, tWifiTry = 0, tMqttTry = 0;

// ---------------- yardımcılar ----------------
static const char* fanModeName() {
  switch (fanMode) { case FAN_ON: return "on"; case FAN_OFF: return "off"; case FAN_PCT: return "pct"; default: return "auto"; }
}
static void addWarn(const char* code) { if (nWarn < 8) warn[nWarn++] = code; }

void readIna(Adafruit_INA219& ina, bool ok, Line& L) {
  if (!ok) { L = Line(); return; }
  float bus = ina.getBusVoltage_V();          // Vin− ile GND arası (yük tarafı)
  float shunt = ina.getShuntVoltage_mV();     // Vin+ − Vin−
  float i = ina.getCurrent_mA() / 1000.0f;
  if (fabsf(i) < 0.002f) i = 0;               // gürültü
  L.vl = bus; L.v = bus + shunt / 1000.0f; L.i = i; L.p = L.v * i;
}

float readTemp(uint8_t idx) {
  if (idx >= nTemp) return NAN;
  float t = ds.getTempC(addrT[idx]);
  return (t == DEVICE_DISCONNECTED_C || t < -55) ? NAN : t;
}

void applyFan() {
  float tmax = NAN;
  if (!isnan(tFC)) tmax = tFC;
  if (!isnan(tEL)) tmax = isnan(tmax) ? tEL : max(tmax, tEL);
  if (isnan(tmax)) fanAutoOn = true;                    // sıcaklık okunamıyorsa güvenli taraf: aç
  else if (tmax >= FAN_ON_C) fanAutoOn = true;
  else if (tmax <= FAN_OFF_C) fanAutoOn = false;
  switch (fanMode) {
    case FAN_ON:  fanPct = FAN_MAX_PCT; break;
    case FAN_OFF: fanPct = 0; break;
    case FAN_PCT: fanPct = min(fanCmdPct, FAN_MAX_PCT); break;
    default:      fanPct = fanAutoOn ? FAN_MAX_PCT : 0;
  }
  FAN_PWM_WRITE((uint32_t)fanPct * ((1u << FAN_PWM_BITS) - 1) / 100);
}

// Teşhis: DS18B20'lerin hangi pine bağlı olduğunu bulur (Seri Monitör'e "tara" yazın).
// I²C (21, 22), UART (1, 3) ve flaş (6–11) pinleri taranmaz.
void scanOneWirePins() {
  const uint8_t pins[] = { 4, 5, 13, 14, 15, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33 };
  Serial.println("[tara] DS18B20 araniyor...");
  bool any = false;
  for (uint8_t p : pins) {
    OneWire ow(p); uint8_t a[8]; uint8_t n = 0;
    ow.reset_search();
    while (n < 4 && ow.search(a)) { if (OneWire::crc8(a, 7) == a[7] && (a[0] == 0x28 || a[0] == 0x10 || a[0] == 0x22)) n++; }
    if (n) { Serial.printf("[tara] GPIO%u: %u sensor\n", p, n); any = true; }
  }
  if (!any) Serial.println("[tara] hicbir pinde sensor bulunamadi: kablo, 4,7 kOhm pull-up ve 3V3/GND baglantisini kontrol edin");
  Serial.printf("[tara] bitti. Su an kullanilan pin: GPIO%d\n", PIN_ONEWIRE);
  FAN_PWM_INIT(); applyFan();                // tarama fan pinini de yokladı: PWM'i yeniden kur
  oneWire.begin(PIN_ONEWIRE);                // asıl 1-Wire hattını geri al
}

void handleFanCmd(String s) {
  s.trim(); s.toLowerCase();
  if (s == "tara" || s == "scan") { scanOneWirePins(); return; }
  if (s == "auto" || s == "oto") fanMode = FAN_AUTO;
  else if (s == "on" || s == "ac" || s == "1") fanMode = FAN_ON;
  else if (s == "off" || s == "kapat" || s == "0") fanMode = FAN_OFF;
  else if (s.length() && isDigit(s[0])) { fanMode = FAN_PCT; fanCmdPct = (uint8_t)constrain(s.toInt(), 0, 100); }
  else { Serial.printf("[fan] bilinmeyen komut: %s\n", s.c_str()); return; }
  Serial.printf("[fan] komut: %s -> mod %s\n", s.c_str(), fanModeName());
  applyFan();
  tPublish = 0;                                          // yeni durumu hemen yayınla
}

void onMqtt(char* topic, byte* payload, unsigned int len) {
  String s; s.reserve(len);
  for (unsigned int k = 0; k < len; k++) s += (char)payload[k];
  if (String(topic) == TOPIC_CMD) handleFanCmd(s);
}

// ---------------- bağlantı (bloklamadan yeniden dener) ----------------
void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - tWifiTry < 10000 && tWifiTry) return;
  tWifiTry = millis();
  Serial.println("[wifi] baglaniliyor...");
  WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  if (millis() - tMqttTry < 5000 && tMqttTry) return;
  tMqttTry = millis();
  String id = "h2ikiz-esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("[mqtt] %s:%d baglaniliyor...\n", MQTT_HOST, MQTT_PORT);
  if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS)) {
    mqtt.subscribe(TOPIC_CMD);
    Serial.println("[mqtt] baglandi, komut konusuna abone olundu");
  } else {
    Serial.printf("[mqtt] basarisiz, durum %d\n", mqtt.state());
  }
}

// ---------------- yayın ----------------
static void putNum(JsonDocument& d, const char* k, float x, uint8_t dec) {
  if (isnan(x)) d[k] = nullptr; else d[k] = serialized(String(x, (unsigned int)dec));
}

void publish() {
  JsonDocument d;
  putNum(d, "pv_v", pv.v, 3); putNum(d, "pv_vl", pv.vl, 3); putNum(d, "pv_i", pv.i, 4); putNum(d, "pv_p", pv.p, 3);
  putNum(d, "fc_v", fc.v, 3); putNum(d, "fc_i", fc.i, 4); putNum(d, "fc_p", fc.p, 4);
  putNum(d, "t_fc", tFC, 1);  putNum(d, "t_el", tEL, 1);
  d["fan"] = fanPct > 0 ? 1 : 0; d["fan_pct"] = fanPct; d["fan_mode"] = fanModeName();
  JsonArray w = d["warn"].to<JsonArray>(); for (uint8_t k = 0; k < nWarn; k++) w.add(warn[k]);
  d["up"] = millis() / 1000; d["rssi"] = WiFi.RSSI();
  char buf[480]; size_t n = serializeJson(d, buf, sizeof(buf));
  // (const uint8_t*) dönüşümü şart: yoksa PubSubClient n'yi "retained" bayrağı sanan başka bir işlevi seçer
  bool ok = mqtt.connected() && mqtt.publish(TOPIC_TELE, (const uint8_t*)buf, (unsigned int)n, false);
  Serial.printf("[yayin] %s %s\n", ok ? "ok" : "YOK", buf);
}

// ---------------- ekran ----------------
void drawOled() {
  if (!okOled) return;
  char l[24];
  oled.clearDisplay(); oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  // sarı bölge (0–15): bağlantı ve fan durumu
  oled.setCursor(0, 0);
  oled.print(WiFi.status() == WL_CONNECTED ? "WiFi OK " : "WiFi -- ");
  oled.print(mqtt.connected() ? "MQTT OK" : "MQTT --");
  snprintf(l, sizeof(l), "Fan %-4s %3u%%", fanModeName(), fanPct); oled.setCursor(0, 8); oled.print(l);
  // mavi bölge (16–63): ölçümler
  snprintf(l, sizeof(l), "PV %4.2fV %5.3fA", isnan(pv.v) ? 0 : pv.v, isnan(pv.i) ? 0 : pv.i); oled.setCursor(0, 18); oled.print(l);
  snprintf(l, sizeof(l), "   %5.2fW", isnan(pv.p) ? 0 : pv.p); oled.setCursor(0, 27); oled.print(l);
  snprintf(l, sizeof(l), "YH %4.2fV %5.3fA", isnan(fc.v) ? 0 : fc.v, isnan(fc.i) ? 0 : fc.i); oled.setCursor(0, 37); oled.print(l);
  oled.setCursor(0, 46);
  if (isnan(tFC)) oled.print("T yh --.-"); else { snprintf(l, sizeof(l), "T yh %4.1f", tFC); oled.print(l); }
  if (isnan(tEL)) oled.print(" el --.-C"); else { snprintf(l, sizeof(l), " el %4.1fC", tEL); oled.print(l); }
  oled.setCursor(0, 56); oled.print(nWarn ? ("! " + warn[0]).c_str() : "Uyari yok");
  oled.display();
}

// DS18B20'leri arar; bulamazsa döngüde 10 s'de bir yeniden denenir
uint32_t tTempScan = 0;
void scanTemps() {
  tTempScan = millis();
  ds.begin(); ds.setWaitForConversion(false); ds.setResolution(11);
  nTemp = min<uint8_t>(ds.getDeviceCount(), 2);
  Serial.printf("DS18B20 (GPIO%d): %u sensor bulundu\n", PIN_ONEWIRE, nTemp);
  for (uint8_t k = 0; k < nTemp; k++) {
    ds.getAddress(addrT[k], k);
    Serial.printf("DS18B20 #%u adres: ", k);
    for (uint8_t b = 0; b < 8; b++) Serial.printf("%02X", addrT[k][b]);
    Serial.println(k == TEMP_FC_INDEX ? "  (yakit hucresi)" : "  (elektrolizor)");
  }
  ds.requestTemperatures();
}

// ---------------- kurulum ----------------
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\nH2 dijital ikiz - ESP32 olcum dugumu");
  Serial.printf("Pinler: SDA %d, SCL %d, DS18B20 %d, fan %d\n", PIN_SDA, PIN_SCL, PIN_ONEWIRE, PIN_FAN);
  Wire.begin(PIN_SDA, PIN_SCL);
  FAN_PWM_INIT(); FAN_PWM_WRITE(0);

  okPV = inaPV.begin(); okFC = inaFC.begin();          // varsayılan kalibrasyon: 32 V, 2 A (0,1 Ω şönt)
  Serial.printf("INA219 PV (0x40): %s, YH (0x41): %s\n", okPV ? "var" : "YOK", okFC ? "var" : "YOK");

  scanTemps();

  okOled = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.printf("OLED: %s\n", okOled ? "var" : "YOK");

  WiFi.mode(WIFI_STA);
  // TLS: bağlantı şifrelenir; sunucu sertifikası doğrulanmaz (basitlik için). Daha sıkı güvenlik için
  // net.setCACert(...) ile sunucunun kök sertifikası verilebilir.
  net.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);
  Serial.println("Seri komutlar: auto | on | off | 0-100 (fan) | tara (DS18B20 pinini bul)");
}

// ---------------- döngü ----------------
void loop() {
  ensureWifi(); ensureMqtt(); mqtt.loop();

  // yerel fan kontrolü: Seri Monitör'den komut
  if (Serial.available()) handleFanCmd(Serial.readStringUntil('\n'));

  if (nTemp < 2 && millis() - tTempScan >= 10000) scanTemps();   // eksik sensör varsa yeniden ara

  if (millis() - tSensor >= SENSOR_MS) {
    tSensor = millis();
    readIna(inaPV, okPV, pv); readIna(inaFC, okFC, fc);
    float t0 = readTemp(TEMP_FC_INDEX), t1 = readTemp(TEMP_FC_INDEX == 0 ? 1 : 0);
    tFC = t0; tEL = t1;
    ds.requestTemperatures();                            // bir sonraki okuma için (bloklamaz)

    nWarn = 0;
    if (!okPV) addWarn("INA_PV_MISSING");
    if (!okFC) addWarn("INA_FC_MISSING");
    if (isnan(tFC)) addWarn("T_FC_MISSING");
    if (isnan(tEL)) addWarn("T_EL_MISSING");
    if (!isnan(tFC) && tFC > T_WARN_C) addWarn("T_FC_HIGH");
    if (!isnan(tEL) && tEL > T_WARN_C) addWarn("T_EL_HIGH");
    if (!isnan(pv.i) && pv.i > I_WARN_A) addWarn("PV_I_HIGH");
    if (!isnan(fc.i) && fc.i > I_WARN_A) addWarn("FC_I_HIGH");

    applyFan(); drawOled();
  }

  if (millis() - tPublish >= PUBLISH_MS || tPublish == 0) {
    tPublish = millis();
    publish();
  }
}
