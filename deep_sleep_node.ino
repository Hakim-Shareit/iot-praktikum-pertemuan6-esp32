/*
 * Praktikum IoT - Pertemuan 6: Evaluasi Deep Sleep
 * Node IoT ESP32 + DHT22 + LED + Buzzer, bertenaga baterai Li-ion 2000 mAh
 *
 * Siklus kerja (1x tiap 10 menit):
 *   bangun -> baca DHT22 -> evaluasi alarm -> sambung Wi-Fi -> kirim data -> deep sleep
 *
 * Catatan:
 *  - Setelah deep sleep, ESP32 RESET: setup() dijalankan dari awal, loop() tidak dipakai.
 *  - millis() ikut reset; pola non-blocking hanya berlaku di jendela aktif.
 *  - Untuk demo di Wokwi, ubah MODE_DEMO menjadi 1 (tidur 15 detik).
 */
#include <WiFi.h>
#include "DHTesp.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#define MODE_DEMO 1   // 1 = tidur 15 detik (demo), 0 = tidur 10 menit (sebenarnya)

// ---------- Pin ----------
const int DHT_PIN    = 15;
const int LED_NORMAL = 2;
const int LED_ALARM  = 4;
const int BUZZER_PIN = 5;

// ---------- Ambang batas ----------
const float AMBANG_SUHU_KRITIS       = 38.0;
const float AMBANG_KELEMBABAN_RENDAH = 20.0;

// ---------- Wi-Fi ----------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
const unsigned long TIMEOUT_WIFI = 5000;   // ms

// ---------- Interval tidur ----------
#if MODE_DEMO
const uint64_t DURASI_TIDUR_US = 15ULL * 1000000ULL;
#else
const uint64_t DURASI_TIDUR_US = 600ULL * 1000000ULL;   // 10 menit
#endif

// Variabel di RTC memory: nilainya bertahan selama deep sleep
RTC_DATA_ATTR int jumlahBangun = 0;

DHTesp dhtSensor;
bool alarmAktif = false;

// ---------- Fungsi bantu ----------
void cetakPenyebabBangun() {
  esp_sleep_wakeup_cause_t sebab = esp_sleep_get_wakeup_cause();
  switch (sebab) {
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Penyebab bangun: timer RTC"); break;
    default:                     Serial.println("Penyebab bangun: power-on / reset"); break;
  }
}

void tungguMillis(unsigned long ms) {      // tunggu tanpa delay()
  unsigned long mulai = millis();
  while (millis() - mulai < ms) { yield(); }
}

bool sambungkanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < TIMEOUT_WIFI) { yield(); }
  return WiFi.status() == WL_CONNECTED;
}

void kirimData(float suhu, float kelembaban, int rssi) {
  // Simulasi pengiriman: pada perangkat nyata ganti dengan HTTP POST / MQTT publish
  Serial.printf("[KIRIM] {\"siklus\":%d,\"suhu\":%.2f,\"rh\":%.2f,\"rssi\":%d,\"alarm\":%s}\n",
                jumlahBangun, suhu, kelembaban, rssi, alarmAktif ? "true" : "false");
}

void evaluasiKondisi(float suhu, float kelembaban) {
  bool suhuKritis       = (suhu > AMBANG_SUHU_KRITIS);
  bool kelembabanRendah = (kelembaban < AMBANG_KELEMBABAN_RENDAH);
  alarmAktif = suhuKritis || kelembabanRendah;

  if (suhuKritis && kelembabanRendah) {
    Serial.println(">>> PERINGATAN [SUHU & KELEMBABAN]! <<<");
  } else if (suhuKritis) {
    Serial.println(">>> PERINGATAN [SUHU]: > 38 C! <<<");
  } else if (kelembabanRendah) {
    Serial.println(">>> PERINGATAN [KELEMBABAN]: < 20 % <<<");
  } else {
    Serial.println("Status: Lingkungan Normal & Terkendali.");
  }
}

void masukDeepSleep() {
  WiFi.disconnect(true);       // matikan radio sebelum tidur
  WiFi.mode(WIFI_OFF);

  // Pertahankan LED merah tetap menyala saat tidur bila alarm aktif
  // (menambah konsumsi arus; hapus blok ini bila ingin hemat maksimal).
  if (alarmAktif) {
    digitalWrite(LED_ALARM, HIGH);
    gpio_hold_en((gpio_num_t)LED_ALARM);
    gpio_deep_sleep_hold_en();
  }

  Serial.printf("Masuk deep sleep selama %llu detik...\n", DURASI_TIDUR_US / 1000000ULL);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(DURASI_TIDUR_US);
  esp_deep_sleep_start();      // tidak kembali; wake-up = reset
}

// ---------- Setup (seluruh siklus kerja ada di sini) ----------
void setup() {
  Serial.begin(115200);
  gpio_hold_dis((gpio_num_t)LED_ALARM);   // lepaskan hold dari siklus sebelumnya
  gpio_deep_sleep_hold_dis();

  jumlahBangun++;
  Serial.printf("\n=== Siklus ke-%d ===\n", jumlahBangun);
  cetakPenyebabBangun();

  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_ALARM, LOW);
  digitalWrite(LED_NORMAL, LOW);

  // 1) Baca sensor (DHT22 perlu jeda sampling minimum)
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  tungguMillis(dhtSensor.getMinimumSamplingPeriod());
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("[Error] Gagal membaca DHT22, kembali tidur.");
    masukDeepSleep();
  }
  Serial.printf("[Sensor Data] Suhu: %.2f C | Kelembaban: %.2f %%\n",
                data.temperature, data.humidity);

  // 2) Evaluasi multi-kondisi
  evaluasiKondisi(data.temperature, data.humidity);
  if (alarmAktif) {
    tone(BUZZER_PIN, 1000);          // bunyi singkat 1 detik sebelum tidur
    tungguMillis(1000);
    noTone(BUZZER_PIN);
  } else {
    digitalWrite(LED_NORMAL, HIGH);
    tungguMillis(500);               // kedipan singkat penanda normal
    digitalWrite(LED_NORMAL, LOW);
  }

  // 3) Sambung Wi-Fi dan kirim data
  bool online = sambungkanWiFi();
  int rssi = online ? WiFi.RSSI() : -65;   // -65 = nilai mock saat offline
  Serial.println(online ? "[Wi-Fi Connected]" : "[Mode Offline] data tidak terkirim");
  if (online) kirimData(data.temperature, data.humidity, rssi);

  Serial.printf("Waktu aktif siklus ini: %lu ms\n", millis());

  // 4) Tidur sampai siklus berikutnya
  masukDeepSleep();
}

void loop() {
  // Tidak digunakan: seluruh proses berjalan di setup() lalu perangkat tidur.
}
