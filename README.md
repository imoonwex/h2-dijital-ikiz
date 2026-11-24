# Güneş–Hidrojen Dijital İkizi (H₂ Üreteç Uzak Deney Seti)

Heliocentris **Dr FuelCell® Professional** deney setinin web tabanlı dijital ikizi.
Zincir: **Işık → PV panel → PEM elektrolizör (H₂/O₂) → gaz depoları → yakıt hücresi → motor + pervane**

🔗 **Canlı uygulama:** https://akyuz11.github.io/h2-dijital-ikiz/

## Hızlı başlangıç
- Tarayıcıda açmak için: yukarıdaki link, ya da `index.html` dosyasına çift tıklayın.
- Kurulum gerekmez. Tek dosyalık HTML/CSS/JS uygulaması (Chart.js CDN üzerinden yüklenir).
- Yerel sunucuyla çalıştırmak isterseniz: `python -m http.server 8000` → http://localhost:8000

## Dosyalar
| Dosya | Açıklama |
|---|---|
| `index.html` | Uygulamanın tamamı (SVG sahne, model, grafikler, TR/EN, açık/koyu tema) |
| `data/deney_verileri.json` | Laboratuvar Excel ölçümleri ve üretici teknik değerleri |
| `H2_Dijital_Ikiz_Tanitim_uyumlu.mp4` | 61 sn tanıtım videosu |
| `H2_Dijital_Ikiz_Kisa_30sn_TR.mp4` | 30 sn kısa tanıtım, Türkçe altyazılı (son sürüm) |
| `H2_Digital_Twin_Short_30s_EN.mp4` | 30 s short demo, English captions (latest version) |
| `docs/Gelistirme_Notlari.md` | Tasarım planı, model açıklaması, geliştirme yazışmaları |

## Model özeti
- **Işık:** 0–16 kademe; kademe 1–16 laboratuvar deney gruplarına (0,5…2,0 A) karşılık gelir. Kademe sayısı `data/deney_verileri.json` içindeki `ozet_20dk` listesinden otomatik çıkar.
- **PV:** Tek diyot modeli; elektrolizör I–V eğrisiyle kesişimi ölçülen akımı verecek şekilde kalibre.
- **Üretici paneli modu:** Işık panelinden gerçek 4 hücreli panele (Voc 2,3 V, Isc 1,0 A) geçilir; I–V grafiğinde iki panel üst üste, ⚡ Verim sekmesinde (Panel ve sıcaklık) kademe kademe karşılaştırma tablosu.
- **Elektrolizör:** Isınma geçişli gerilim (≈2,8 V → 2,4 V); H₂ debisi ölçüm tablosundan interpolasyon, Faraday teorik değeriyle karşılaştırma.
- **Yakıt hücresi:** Çift hücre, seri veya paralel bağlanabilir; iç direnç 0,175 Ω ile üretici değerlerine (seri 0,8–1,8 V, en fazla 1,7 W) kalibre.
- **Yük:** Motor + pervane, 0,3–100 Ω direnç veya açık devre. "Otomatik tarama" 10 direnci sırayla bağlayıp I–V ve güç eğrisini çıkarır.
- **Motor/pervane:** ≤3 V, ≤130 mA; birinci dereceden atalet.
- Ayrıntılar: uygulamadaki **Model notları** sekmesi ve `docs/Gelistirme_Notlari.md`.

## Bilinen notlar
- Güncel veri: 16 grup, 0,5–2,0 A, 20 dk'da 83–329 mL H₂ (Devre_Tasarimi_Veriler_guncel, 13.11.2026).
- 0,5 / 0,6 / 0,8 / 0,9 / 1,8 / 2,0 A gruplarında ölçülen hacim Faraday sınırına ulaşıyor veya aşıyor (%109, %101, %112, %103, %102, %108) — derste tartışma konusu. 1,4–1,7 A arasında verim %89–95.
- Depo ölçeği 0–400 mL.
- Adres sonuna `?record` eklenirse video kaydı için kare kare kayıt modu açılır.

## Öğrenciler için: nasıl katkı yaparım?
1. Sağ üstten **Fork** edin (kendi hesabınıza kopya).
2. Kendi kopyanızda değişiklik yapın (`index.html` içinde fizik modeli `model`, metinler `i18n` sözlüğünde).
3. Fork'unuzda **Settings → Pages → Branch: main** ile kendi canlı sürümünüzü yayınlayın.
4. Ana projeye eklenmesini istediğiniz değişiklik için **Pull Request** açın.

### Geliştirme fikirleri
- [x] İngilizce altyazılı / 30 sn kısa tanıtım videosu (`H2_Dijital_Ikiz_Kisa_30sn_TR.mp4`, `H2_Digital_Twin_Short_30s_EN.mp4`)
- [x] Yük direnci seçimi (0,3–100 Ω) ve yakıt hücresi I–V / güç eğrisi deneyi
- [x] Paralel/seri yakıt hücresi bağlantı seçeneği
- [x] Gerçek PV paneli (4 hücre, Voc 2,3 V) ile karşılaştırma modu
- [x] Ölçüm verisinin `data/deney_verileri.json` dosyasından dinamik yüklenmesi (yerel sunucu/Live Server veya GitHub Pages gerekir; dosyaya çift tıklanınca gömülü yedek veri kullanılır)
- [x] Öğrenci deney föyü / quiz modu ("Deney föyü ve quiz" sekmesi: 3 rehberli deney, otomatik kontrol, 10 soruluk quiz, yazdır/PDF)
- [x] Konu anlatımı ("📘 Konu anlatımı" sekmesi, sayfa açılınca ilk görünen): 8 ders — enerji zinciri, güneş pili ve I–V eğrisi, elektroliz ve Faraday yasası, hidrojen depolama, yakıt hücresi, seri/paralel bağlantı, yük direnci ve en yüksek güç, verim. Her derste çizim, bu düzenekten sayısal örnek ve "Simülasyonda dene" düğmesi var.
- [x] Başlangıç rehberi (Konu anlatımının ilk sayfası; üstteki "❓ Nasıl kullanılır?" düğmesi): sayfa krokisi, 8 adımlık ilk deney, her adımda ilgili bölümü parlatan "Göster" düğmesi ve "Hızlı başlat"
- [x] Tanıtım videosu düğmesi (üst menü "▶ Tanıtım videosu"; seçili dile göre TR/EN video açılır pencerede oynar)
- [x] Canlı enerji akışı (Sankey) diyagramı (⚡ Verim sekmesi, Genel bakış) ; diyagramın altında açılır "Sankey diyagramı nasıl okunur?" açıklaması
- [x] Panel açısı (θ, 0–90°): sahnenin altındaki çubuktan hazır açılarla (0/15/…/90°), kaydırıcıyla ya da sayıyla girilir; panel hedef açıya yumuşakça döner; panele düşen ışık cos θ ve camdan yansıma (IAM) ile azalır, akım, H₂ üretimi, verim ve tablolar buna göre hesaplanır. Sahnede panel döner, hüzme daralır ve "Üstten görünüş" şeması açıyı gösterir. 0°'de laboratuvar verisi birebir korunur.
- [x] ⚡ Verim sekmesi (eski "Faraday verimi" ve "Verim hesabı" sekmeleri birleşti; alt sekmeler: Genel bakış · Faraday · Panel ve sıcaklık · Yakıt hücresi): (1) PV verimi adım adım (panel boyutu girilir: en × boy → alan; G·cos θ·A; U·I), en yüksek güç noktası, dolum faktörü, uyum oranı ve "fotovoltaik sistemin verimi nasıl hesaplanır" açıklaması; (2) panel sıcaklığı (−5/+5 °C, 25–100 °C hazır değerler): "25 °C'de 100 → T'de X" karşılaştırması, verim–sıcaklık grafiği, sıcaklık katsayısı β, NOCT tahmini; (3) elektrolizör Faraday ve enerji verimi hesap makinesi (kendi ölçümünüz girilebilir); (4) yakıt hücresi verimi; (5) toplam verim. Sıcaklık bütün simülasyona uygulanır (25 °C'de laboratuvar verisi birebir korunur); ölçüm tablosu ve CSV'de sıcaklık sütunu var.
- [x] Sekme simgesi (`favicon.svg`, `apple-touch-icon.png`) ve paylaşım önizlemesi (`og-image.png`, Open Graph etiketleri)
