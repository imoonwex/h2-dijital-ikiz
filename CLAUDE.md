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
- `favicon.svg`, `apple-touch-icon.png`, `og-image.png` — sekme simgesi ve paylaşım önizlemesi. `og:url`/`og:image` mutlak adresleri imoonwex.github.io'yu gösterir; hocanın deposuna PR açılırsa bunlar akyuz11 adresine çevrilmeli.

## Çalıştırma
JSON `fetch` ile okunduğu için dosyaya çift tıklamak yetmez: VS Code **Live Server** (sağ tık → Open with Live Server, port 5500) veya `python -m http.server`.

## Model (index.html içinde)
- Işık kademeleri: `LEVEL_I`/`IPH` → `calibrate()` ile `LAB20`'den üretilir; kademe n = grup n akımı. `NLV` = kademe sayısı (16).
- PV: tek diyot. Model paneli `PV` (6 hücre eşdeğer, n=1,3, IscRef 2,2 A); üretici paneli `PVR` (4 hücre, Voc 2,3 V, Isc 1,0 A, n=1,7). `S.pv` = "m" | "r". Işık akımı `iphAt(lv, P)`.
- Elektrolizör: `vEl(I, c)`; soğukluk payı 0,4·(1−c)·min(1, I/0,5). H₂ debisi `h2Rate(I)` = ölçümlerin doğrusal ara değerlemesi. Isınma eşiği I > 0,01 A.
- Yakıt hücresi: `fcCellV(I)` = 0,98 − 0,03·ln(1+I/2 mA) − 0,175·I (seride 1,70 W @ 0,80 V, üretici değeri). `stackV(I,s,conn)`, `solveFc(s, conn, load)`. `S.conn` = "s"|"p"; `S.load` = "motor" | sayı (Ω) | "open". Dirençler `RES` = 0,3…100 Ω.
- Doğrulama: model paneli, ısınmış elektrolizörle her kademede ölçülen 20 dk hacmini birebir vermeli (83 … 329 mL). Model değişikliğinden sonra bunu kontrol et.
- Test kancası: `window.__twin` (S, step, setLevel, solveFc, setLoad, setConn …). `?record` → kare kare kayıt modu (`__twin.tick`).

## Sekmeler (alt bölüm)
📘 Konu anlatımı (`#tp6`, varsayılan açık; `LESSONS` dizisi: [0] numarasız başlangıç rehberi (`guide: true`, `LESSONS.unshift`, "Göster" düğmeleri `data-show` → `showSpot`), ardından 8 ders; üstteki `#btnHelp` rehberi açar, ilk ziyarette localStorage `h2twin-seen` ile bir kez ipucu çıkar) · Faraday ve verim sekmesinin başında canlı `sankeySvg()` ve altında açılır açıklama (`SK_HOW`). Sankey ayrı bir ders olmasın — kullanıcı açıklamanın diyagramın yanında, sekmede durmasını istedi. Dil ve tema sol üstteki ☰ menüsünde (`#btnMenu`, `#langSeg`, `#themeSeg`, `syncMenu()`); sekme adlarında emoji var · Grafikler (`#tp1`, Chart.js CDN; c0–c4) · Ölçüm tablosu · Faraday ve verim (+ PV karşılaştırma tablosu) · Deney föyü ve quiz (`#tp5`; `WS_TXT`, `QUIZ`, localStorage `h2twin-ws`) · Model notları (`NOTES`, kartlar + açılır formüller).

## Kurallar
- **Her metin iki dilli**: arayüz metinleri `I18N.tr` / `I18N.en` (anahtar + `data-i18n`), dersler/föy kendi tr/en nesnelerinde. Birini eklersen ötekini de ekle.
- Renkler CSS değişkenleri (`--accent`, `--h2`, `--o2`, `--sun`, `--ok`, `--warn` …); koyu temada da kontrol et. Global `.lab` sınıfı sahne düzenidir — yeni sınıflara bu adı verme.
- Sayılar Türkçede virgüllü: `num(x, d)`, `pct(...)`.
- Laboratuvar verisinde 6 grup Faraday sınırını aşıyor (0,5/0,6/0,8/0,9/1,8/2,0 A) — bu bilinçli olarak olduğu gibi bırakıldı ve notlarda/derslerde tartışma konusu olarak anlatılıyor; "düzeltme".
- Değişiklikten sonra: tarayıcı konsolunda hata olmadığını, TR/EN ve açık/koyu temayı, telefon genişliğini kontrol et.

## Git akışı (kullanıcı için)
Değişiklikten sonra: Source Control → mesaj → Commit → Sync Changes. Oturum başında önce Sync Changes.
Sıradaki olası iş: değişiklikleri hocanın deposuna pull request olarak önermek (GitHub → Contribute → Open pull request). README'deki "Canlı uygulama" linki hocanın sitesini (akyuz11) gösterir; bilerek değiştirilmedi.
