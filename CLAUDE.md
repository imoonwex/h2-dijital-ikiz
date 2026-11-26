# CLAUDE.md — H2 dijital ikiz (bitirme projesi)

Bu dosya Claude Code'un projeyi tanıması içindir. Kullanıcı (imoonwex) Türkçe konuşur ve Git/VS Code'da yenidir:
**cevapları Türkçe ver, adımları ayrıntılı ve sade anlat, commit/push'u kullanıcı istemeden yapma.**

## Proje
Güneş–Hidrojen (Dr FuelCell® Professional) uzak deney setinin tarayıcıda çalışan dijital ikizi.
Zincir: ışık → PV paneli → PEM elektrolizör → H₂/O₂ deposu → PEM yakıt hücresi → yük (motor+pervane / direnç).
Bu depo hocanın projesinin (akyuz11/h2-dijital-ikiz) fork'u. Canlı sürüm: https://imoonwex.github.io/h2-dijital-ikiz/

## Dosyalar
- `index.html` — uygulamanın tamamı (SVG sahne, model, grafikler, TR/EN, açık/koyu tema). Tek dosya; derleme yok.
- `data/deney_verileri.json` — laboratuvar ölçümleri (16 grup, 0,5–2,0 A). `ozet_20dk` = [akım A, 20 dk H₂ mL]. Uygulama bunu `loadData()` ile okur; okunamazsa `LAB20` içindeki gömülü yedeği kullanır.
- `H2_Dijital_Ikiz_Kisa_30sn_TR.mp4`, `H2_Digital_Twin_Short_30s_EN.mp4` — 30 sn tanıtım (altyazılı, efekt sesli; seslendirme yok — kullanıcı böyle istedi). Uygulamada üst menüdeki "▶ Tanıtım videosu" düğmesi (`#btnVid`) bunları `<dialog id="vidDlg">` içinde, seçili dile göre (`VIDS`) oynatır.
- `README.md` — geliştirme listesi (hepsi [x]).
- `firmware/h2_ikiz_esp32/` — ESP32 ölçüm düğümü (Arduino): INA219 0x40 PV (Vin+ panel, Vin− elektrolizör), 0x41 yakıt hücresi; DS18B20 ×2 (GPIO4, `TEMP_FC_INDEX`); SSD1306 0x3C; fan GPIO27 PWM en çok %55 (`FAN_MAX_PCT`, set fanı 3 V). Her 5 s JSON yayınlar, fan komutu alır (auto/on/off/0–100). Şifreler `secrets.h`'te (**.gitignore'da, asla commit edilmez**; şablon `secrets.example.h`). Kurulum adımları `firmware/README.md`.
- `favicon.svg`, `apple-touch-icon.png`, `og-image.png` — sekme simgesi ve paylaşım önizlemesi. `og:url`/`og:image` mutlak adresleri imoonwex.github.io'yu gösterir; hocanın deposuna PR açılırsa bunlar akyuz11 adresine çevrilmeli.

## Çalıştırma
JSON `fetch` ile okunduğu için dosyaya çift tıklamak yetmez: VS Code **Live Server** (sağ tık → Open with Live Server, port 5500) veya `python -m http.server`.

## Model (index.html içinde)
- Işık kademeleri: `LEVEL_I`/`IPH` → `calibrate()` ile `LAB20`'den üretilir; kademe n = grup n akımı. `NLV` = kademe sayısı (16).
- PV: tek diyot. Model paneli `PV` (6 hücre eşdeğer, n=1,3, IscRef 2,2 A); üretici paneli `PVR` (4 hücre, Voc 2,3 V, Isc 1,0 A, n=1,7). `S.pv` = "m" | "r". Işık akımı `iphAt(lv, P, ang)`.
- Panel açısı `S.ang` (0–90°, 0° = lambaya dik = laboratuvar konumu): `angF(θ)` = cos θ · IAM, IAM = 1 − 0,05·(1/cos θ − 1). `iphAt`'in `ang` parametresi varsayılan 0'dır: dersler ve föy laboratuvar konumunu kullanır. `S.ang` hedef açı, `S.angNow` panelin o anki açısı: panel `turnPanel()` ile hedefe yumuşakça döner (animate içinde, gerçek zamanla, ≈1–1,7 s; kullanıcı panelin birden dönmesini istemedi). Canlı hesaplar (`step`, PV grafiği, verim, kayıtlar) `S.angNow`, PV karşılaştırma tablosu `S.ang` kullanır; `setAngle(v, true)` beklemeden kurar (hızlı başlat, 1. ders). Verimde panele düşen güç G·cos θ·A. Arayüz: sahnenin altındaki **Panel çubuğu** `#angBar` (kullanıcı panelin yanında, solda olmasını istedi): panel tipi `#pvSeg`, sıcaklık (`#tBarVal`, `#tBarRng`, −5/+5 `data-dt`, `#tInfo`), açı; hazır açılar `#angPre`, `#angRng`/`#angNum` → `setAngle()`; sahnede `drawAngle()` panel yüzeyini (`#pvFace`) daraltır, hüzmeyi kısaltır ve sol üstte "Üstten görünüş" şemasını (`#angView`) çizer. Ölçüm tablosu/CSV'de açı sütunu var.
- Panel sıcaklığı `S.Tp` (0–100 °C, 25 = referans = laboratuvar): `pvAt(P, Tc)` sıcaklığa göre yeni panel nesnesi verir (a ∝ T, I₀ ∝ T³·exp[Eg/(n k)(1/T₂₅−1/T)], Isc +0,05 %/°C; Voc ≈ −2,2 mV/°C/hücre, Pmpp ≈ −0,47 %/°C). `curPV()`/`otherPV()` bunu `S.Tp` ile çağırır; 25 °C'de çarpanlar tam 1. Panel boyutu `PV.dim`/`PVR.dim` (cm, en × boy; varsayılan `dim0` = 25×18 ve 20×15) → `area`; alan akımı değil yalnızca verimi değiştirir.
- ⚡ Verim sekmesi (`#tp3`; kullanıcı "Faraday verimi" ve "Verim hesabı"nı tek sekmede birleştirmek istedi, yeri Deney föyünden önce). Alt sekmeler `#effSub` → `setEffSub(k)`: `ov` Genel bakış (Sankey, verim kartları, `#calcTot`), `far` Faraday (lab tablosu + `#calcEL`), `pv` Panel ve sıcaklık (`#calcPV`, `#calcT`, PV karşılaştırma tablosu), `fc` Yakıt hücresi (`#calcFC`). `renderCalc()` kartları bu kaplara yazar, `updateCalc()` canlı sayılar (updateUi'den her 0,2 s, sekme açıkken). Her kartta önce sonuç kutuları (`tile()`), sonra adım adım hesap (`li()`): PV adımları + `pvMpp()`, sıcaklık (`setTemp`, `TEMP_PRE`, grafik `plot()`), elektrolizör hesap makinesi (`elc`, `follow` = simülasyonu izle), yakıt hücresi, toplam. Açıklama metinleri `PVSYS`, `TEMPX` (tr/en). Kullanıcının hocası istedi: verim hesabı, panel boyutu, PV sistem verimi, sıcaklıkla verim.
- Otomatik ısınma `S.autoT` (`#tAuto`, varsayılan kapalı): `step()` içinde T → 25 + (NOCT−20)/800·G·cos θ, zaman sabiti `HEAT_TAU` = 420 s (sim zamanı). Bu modda `S.Tp` ondalıklıdır; ekranda her yerde `Math.round(S.Tp)` gösterilir. Elle sıcaklık ayarı (`setTemp`) otomatik ısınmayı kapatır.
- Deney föyü beklenen değerleri (`wsTargets`) laboratuvar koşulundadır (0°, 25 °C). `labCondOk()` değilse föyde `#wsWarn` uyarısı ve `labCondition()` düğmesi çıkar.
- Ölçümler (`records`) ve I–V noktaları (`ivPts`) localStorage `h2twin-rec`'te saklanır (`saveRec()`), açılışta geri yüklenir.
- Verim › Genel bakış: eski `#effGrid` kartları kaldırıldı; beş verim kutusu + toplam formülü `#kTot`'ta (updateCalc).
- 📡 Canlı ölçüm (`LIVE`, `liveCfg`, `liveUse()`): üstte `#btnLive` → `#liveDlg` (sunucu, kullanıcı, anahtar, konular; varsayılan Adafruit IO `wss://io.adafruit.com:443/mqtt/`, `{user}/feeds/h2-olcum` ve `{user}/feeds/h2-fan`). mqtt.js tembel yüklenir (unpkg mqtt@4.3.7). Anahtar koda yazılmaz; localStorage `h2twin-live`'a yalnızca "hatırla" seçilirse yazılır. Taze veri (< 20 s) ve `LIVE.use` ise `step()` PV (`pv_i`, `pv_v`) ve yakıt hücresi (`fc_i`, `fc_v`) değerlerini ölçümden alır, H₂ üretimi `h2Rate(ölçülen I)`. Panel `#pnlLive` (konsolun başında, yalnızca bağlanınca), sahne rozeti `#liveBadge`, uyarı kodları `WARN_TXT`, fan komutu `liveFan()`. Kayıtlarda `live` bayrağı (tabloda 📡).
- Elektrolizör: `vEl(I, c)`; soğukluk payı 0,4·(1−c)·min(1, I/0,5). H₂ debisi `h2Rate(I)` = ölçümlerin doğrusal ara değerlemesi. Isınma eşiği I > 0,01 A.
- Yakıt hücresi: `fcCellV(I)` seçili modeli çağırır: `S.fcm` = "lab" (varsayılan) → `fcLabV` = min(OCV, E0 − A·ln(I/I0) − R·I), `FCLAB` = {E0 0,8252, A 0,02197, I0 1,081 mA, R 0,3433 Ω, OCV 0,87} (26.11.2026 Hücre 1 ölçümü `FCLAB_PTS`, RMSE 6,4 mV, geçerli 7,8–800 mA; iki hücre aynı varsayılır) | "mfr" → `fcMfrV` = 0,98 − 0,03·ln(1+I/2 mA) − 0,175·I (veri sayfası, seride 1,70 W). Seçim "Yakıt hücresi ve yük" panelinde `#fcmSeg` → `setFcm()`. Ölçülen hücre veri sayfasının ≈ %40'ı (seride ≈ 0,67 W @ 0,73 Ω). Yakıt kullanım verimi `FC_UTIL` (η_F, varsayılan 1,0, ölçülmedi; Verim › Yakıt hücresi'nde `#fcUtil` kaydırıcısı): tüketim = ΣI/(2F·η_F). Derslerdeki yakıt hücresi sayıları `fcMot()`, `fcMpp()`, `fcH2()` ile seçili modelden hesaplanır — sabit sayı yazma. Laboratuvar kartı `renderFcLab()` (tablo, `FCLIM` sınırlamalar). JSON'da `yakit_hucresi_olcum` bloğu. `stackV(I,s,conn)`, `solveFc(s, conn, load)`. `S.conn` = "s"|"p"; `S.load` = "motor" | sayı (Ω) | "open". Dirençler `RES` = 0,3…100 Ω.
- Doğrulama: model paneli, ısınmış elektrolizörle her kademede ölçülen 20 dk hacmini birebir vermeli (72 … 280 mL). Model değişikliğinden sonra bunu kontrol et.
- Test kancası: `window.__twin` (S, step, setLevel, solveFc, setLoad, setConn …). `?record` → kare kare kayıt modu (`__twin.tick`).

## Sekmeler (alt bölüm)
📘 Konu anlatımı (`#tp6`, varsayılan açık; `LESSONS` dizisi: [0] numarasız başlangıç rehberi (`guide: true`, `LESSONS.unshift`, "Göster" düğmeleri `data-show` → `showSpot`), ardından 8 ders; üstteki `#btnHelp` rehberi açar, ilk ziyarette localStorage `h2twin-seen` ile bir kez ipucu çıkar) · ⚡ Verim sekmesinin Genel bakış alt sekmesinde canlı `sankeySvg()` ve altında açılır açıklama (`SK_HOW`). Sankey ayrı bir ders olmasın — kullanıcı açıklamanın diyagramın yanında, sekmede durmasını istedi. Dil ve tema sol üstteki ☰ menüsünde (`#btnMenu`, `#langSeg`, `#themeSeg`, `syncMenu()`); sekme adlarında emoji var · Grafikler (`#tp1`, Chart.js CDN; c0–c4) · Ölçüm tablosu · ⚡ Verim (yukarıya bkz.) · Deney föyü ve quiz (`#tp5`; `WS_TXT`, `QUIZ`, localStorage `h2twin-ws`; quiz 13 soru, verim ağırlıklı (kullanıcı ölçüm ağırlıklı soruları istemedi): panel verimi/alan/açı/sıcaklık/MPP, Faraday ve enerji verimi, yakıt hücresi, toplam, Sankey, neden hidrojen. Doğru şıklar karışık dağıtılır. Soru listesi değişince `QUIZ_VER` artırılır → eski quiz cevapları silinir) · Model notları (`NOTES`, kartlar + açılır formüller).

## Kontrollerin yerleri (kullanıcıyla kararlaştırıldı)
- Sahnenin altı **Panel** çubuğu: panel tipi, sıcaklık, açı (panelin özellikleri bir arada).
- Sağ konsol: Işık şiddeti (yalnızca ışık) · Zaman (hız, duraklat, sıfırla) · Yakıt hücresi ve yük (**H₂ vanası** burada, seri/paralel, yük, I–V) · Kronometre.
- "Grafikte diğer paneli göster" (`#pvCmp`) Grafikler sekmesinde PV I–V grafiğinin altında.
- Sekme sırası: Konu anlatımı · Grafikler · Ölçüm tablosu · ⚡ Verim (alt sekmeli) · Deney föyü ve quiz · Model notları.

## Kurallar
- **Her metin iki dilli**: arayüz metinleri `I18N.tr` / `I18N.en` (anahtar + `data-i18n`), dersler/föy kendi tr/en nesnelerinde. Birini eklersen ötekini de ekle.
- Renkler CSS değişkenleri (`--accent`, `--h2`, `--o2`, `--sun`, `--ok`, `--warn` …); koyu temada da kontrol et. Global `.lab` sınıfı sahne düzenidir — yeni sınıflara bu adı verme.
- Sayılar Türkçede virgüllü: `num(x, d)`, `pct(...)`.
- Laboratuvar verisi: ilk ölçümde 6 grup (0,5/0,6/0,8/0,9/1,8/2,0 A) Faraday sınırını aşıyordu; kullanıcı bu grupları 26.11.2026'da yeniden ölçtü (72, 90, 118, 135, 260, 276 mL; 1,8 ve 2,0 A'de 10 dk ölçülüp ×2). Artık 16 grubun η_F'si %89–99. 2,0 A (276) 1,9 A'den (280) biraz düşük — ölçüm belirsizliği, olduğu gibi bırakıldı. Kullanıcı, yeni bir ölçüm %100'ü aşarsa küçük bir yuvarlamayla düzeltilmesine izin verdi.
- Değişiklikten sonra: tarayıcı konsolunda hata olmadığını, TR/EN ve açık/koyu temayı, telefon genişliğini kontrol et.

## Git akışı (kullanıcı için)
Değişiklikten sonra: Source Control → mesaj → Commit → Sync Changes. Oturum başında önce Sync Changes.
Sıradaki olası iş: değişiklikleri hocanın deposuna pull request olarak önermek (GitHub → Contribute → Open pull request). README'deki "Canlı uygulama" linki hocanın sitesini (akyuz11) gösterir; bilerek değiştirilmedi.
