# ESP32 ölçüm düğümü → dijital ikiz (MQTT)

ESP32, PV hattındaki ve yakıt hücresi hattındaki INA219'lardan gerilim, akım ve gücü, iki DS18B20'den sıcaklığı okur. OLED'de gösterir, fanı sürer ve her 5 saniyede bir ölçümleri **MQTT** ile buluta gönderir. Dijital ikiz (üst menüde **📡 Canlı**) aynı yere bağlanır, ölçümleri canlı gösterir ve fana komut gönderir.

```
ESP32 ──MQTT (TLS 8883)──▶ Adafruit IO ◀──MQTT (WebSocket 443)── Dijital ikiz (tarayıcı)
         h2-olcum  (ölçüm, JSON)  ─────────────────▶
         h2-fan    (fan komutu)   ◀─────────────────
                  └─ ölçümler 30 gün saklanır (Adafruit IO, ücretsiz plan)
```

> 🔒 **Şifreler hiçbir zaman koda yazılmaz.** ESP32'nin şifreleri `secrets.h` dosyasındadır ve bu dosya `.gitignore`'da olduğu için GitHub'a gitmez. İkizde kullanıcı adı ve anahtar yalnızca bağlanan kişinin tarayıcısında girilir.

## 1. Adafruit IO hesabı (bulut)

1. <https://io.adafruit.com> adresinde ücretsiz hesap açın.
2. **Feeds → New Feed** ile iki feed oluşturun: `h2-olcum` ve `h2-fan`. ESP32 ilk mesajı gönderdiğinde feed kendiliğinden de oluşabilir.
3. Sağ üstteki **🔑 (Key)** simgesine basın. **Username** ve **Active Key** değerlerini not edin. Bu anahtar şifre gibidir; kimseyle paylaşmayın, hiçbir dosyaya yazıp GitHub'a göndermeyin.

Ücretsiz plan: dakikada en çok 30 mesaj (ESP32 5 s'de bir = dakikada 12), 10 feed, veriler 30 gün saklanır. Feed sayfasından grafik görülebilir ve CSV indirilebilir.

## 2. Arduino IDE

1. **Dosya → Tercihler → Ek kart yöneticisi adresleri**: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. **Araçlar → Kart → Kart Yöneticisi**: "esp32" (Espressif) kurun. Kart olarak **ESP32 Dev Module** seçin.
   > ⚠️ **Sürüm 3.3.x kullanın, 4.0.0 kullanmayın.** 4.0.0 ile OneWire kütüphanesi çalışmıyor: DS18B20'ler bulunamıyor ("0 sensor bulundu"), 1-Wire hattı HIGH olsa bile. 26.11.2026'da denendi; 3.3'e düşünce iki sensör de okundu. Arduino IDE güncelleme önerirse esp32 paketini güncellemeyin.
3. **Araçlar → Kütüphaneleri Yönet** ile şunları kurun:
   - Adafruit INA219
   - Adafruit SSD1306 ve Adafruit GFX Library
   - OneWire ve DallasTemperature
   - PubSubClient (Nick O'Leary)
   - ArduinoJson (Benoit Blanchon, **sürüm 7**)

## 3. Şifre dosyası

1. `firmware/h2_ikiz_esp32/secrets.example.h` dosyasını aynı klasöre **`secrets.h`** adıyla kopyalayın.
2. `secrets.h` içine Wi-Fi adını ve şifresini, Adafruit IO kullanıcı adını ve anahtarını yazın. ESP32 yalnızca **2,4 GHz** ağlara bağlanır.
3. Kontrol: VS Code'da **Source Control** listesinde `secrets.h` **görünmemeli**. Görünürse commit etmeyin ve bana haber verin.

## 4. Yükleme ve ilk kontrol

1. `h2_ikiz_esp32.ino` dosyasını Arduino IDE'de açın, ESP32'yi USB ile bağlayın, **Yükle**'ye basın.
2. **Seri Monitör**'ü açın (115200 baud). Şunları görmelisiniz:
   - `INA219 PV (0x40): var, YH (0x41): var`
   - İki `DS18B20 #… adres` satırı ve hangisinin yakıt hücresi sayıldığı
   - `[wifi] …`, ardından `[mqtt] baglandi`
   - 5 saniyede bir `[yayin] ok {…}`
3. **Sıcaklık sensörlerinin yeri:** Yakıt hücresindeki sensörü elinizle ısıtın. Seri Monitör'de `t_fc` değeri artmalı. `t_el` artıyorsa programın başındaki `TEMP_FC_INDEX` değerini `0`'dan `1`'e çevirip yeniden yükleyin.
4. **Fan:** Seri Monitör'e `on`, `off`, `auto` ya da `40` (yüzde) yazarak deneyin. Program PWM'i en çok %55'te tutar.

## 5. İkize bağlanma

1. Canlı sitede (ya da Live Server'da) üst menüdeki **📡 Canlı** düğmesine basın.
2. **Kullanıcı adı** ve **Anahtar** alanlarına Adafruit IO bilgilerinizi girin. Sunucu ve konular varsayılan olarak doğrudur.
3. **Bağlan**'a basın. Birkaç saniye içinde başlıktaki nokta yeşile döner ve sağ konsolda **📡 Canlı ölçüm** paneli belirir.
4. "Simülasyon yerine ölçümleri kullan" açıkken PV ve yakıt hücresinin gerilim ve akımı ölçümden gelir, H₂ üretimi ölçülen akımdan modelle hesaplanır. Sahnede "📡 Gerçek ölçüm kullanılıyor" yazar. 20 s ölçüm gelmezse ikiz kendiliğinden simülasyona döner.
5. Paneldeki **Oto / Aç / Kapat** düğmeleri fana komut gönderir; ESP32 birkaç saniye içinde uygular ve yeni durumu yayınlar.

## Gönderilen veri (JSON)

| Alan | Anlamı |
|---|---|
| `pv_v`, `pv_vl` | PV gerilimi: panel ucu (Vin+) ve elektrolizör ucu (Vin−), V |
| `pv_i`, `pv_p` | PV akımı (A) ve gücü (W) |
| `fc_v`, `fc_i`, `fc_p` | Yakıt hücresi gerilimi (V), akımı (A), gücü (W) |
| `t_fc`, `t_el` | Yakıt hücresi ve elektrolizör sıcaklığı (°C; okunamazsa `null`) |
| `fan`, `fan_pct`, `fan_mode` | Fan açık mı (0/1), PWM yüzdesi, mod (`auto` / `on` / `off` / `pct`) |
| `warn` | Uyarı kodları: `T_FC_HIGH`, `T_EL_HIGH` (> 55 °C), `PV_I_HIGH`, `FC_I_HIGH` (> 1,9 A), `INA_PV_MISSING`, `INA_FC_MISSING`, `T_FC_MISSING`, `T_EL_MISSING` |
| `up`, `rssi` | Açılıştan beri geçen süre (s), Wi-Fi sinyali (dBm) |

## Sorun giderme

- **`[mqtt] basarisiz, durum 5`**: kullanıcı adı ya da anahtar yanlış.
- **`durum -2`**: internet yok ya da sunucu adı yanlış.
- **Mesajlar bir süre gelip sonra kesiliyor**: Adafruit IO dakikada 30 mesaj sınırı aşılmış olabilir. `PUBLISH_MS` değerini büyütün.
- **İkiz "Bağlı, ölçüm bekleniyor" diyor ama veri gelmiyor**: ikizdeki ölçüm konusu ile `secrets.h`'teki `TOPIC_TELE` aynı olmalı (`kullanici/feeds/h2-olcum`).
- **Değerler ters işaretli**: INA219'un Vin+ ve Vin− uçları ters bağlanmış.
- **TLS notu**: Bağlantı şifrelidir, ama basitlik için sunucu sertifikası doğrulanmaz (`net.setInsecure()`). Daha sıkı güvenlik için `net.setCACert(...)` ile sunucunun kök sertifikası verilebilir.
