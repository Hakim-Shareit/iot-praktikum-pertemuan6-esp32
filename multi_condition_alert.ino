/*
 * Praktikum IoT - Pertemuan 6: Embedded System Simulator (Wokwi)
 * Node IoT ESP32 + DHT22 + LED (hijau/merah) + Piezo Buzzer
 *
 * Fitur:
 *  - Pembacaan sensor non-blocking dengan millis() (interval 2 detik)
 *  - Multi-condition alert: suhu > 38 C ATAU kelembaban < 20 %
 *  - Pesan Serial Monitor berbeda untuk tiap pemicu alarm
 *  - Wi-Fi dengan timeout; fallback ke mode stand-alone (RSSI mock)
 */
#include <WiFi.h>
#include "DHTesp.h"

// ---------- Definisi Pin Periferal ----------
const int DHT_PIN    = 15;  // Data DHT22
const int LED_NORMAL = 2;   // LED hijau (status normal)
const int LED_ALARM  = 4;   // LED merah (alarm)
const int BUZZER_PIN = 5;   // Piezo buzzer

// ---------- Batas Kritis (Threshold) ----------
const float AMBANG_SUHU_KRITIS      = 38.0; // derajat Celcius
const float AMBANG_KELEMBABAN_RENDAH = 20.0; // persen RH
const int   NADA_ALARM_HZ           = 1000; // frekuensi buzzer

// ---------- Konfigurasi Wi-Fi Virtual Wokwi ----------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
const unsigned long timeoutWiFi = 5000;     // ms
const int RSSI_MOCK  = -65;                 // dBm, dipakai saat offline

DHTesp dhtSensor;

// ---------- Penjadwalan Non-blocking ----------
unsigned long waktuSebelumnya = 0;
const unsigned long intervalBaca = 2000;    // baca sensor tiap 2 detik

// ---------- Fungsi Bantu ----------
void sambungkanWiFi() {
  Serial.print("Memeriksa koneksi Wi-Fi: ");
  WiFi.begin(ssid, password);

  unsigned long mulai = millis();
  unsigned long titikTerakhir = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < timeoutWiFi) {
    if (millis() - titikTerakhir >= 200) {  // cetak titik tanpa delay()
      titikTerakhir = millis();
      Serial.print(".");
    }
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi Connected!] IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[Mode Offline / Stand-alone] Melanjutkan simulasi tanpa gateway internet.");
  }
}

void setStatusNormal() {
  digitalWrite(LED_ALARM, LOW);
  digitalWrite(LED_NORMAL, HIGH);
  noTone(BUZZER_PIN);
}

void setStatusAlarm() {
  digitalWrite(LED_ALARM, HIGH);
  digitalWrite(LED_NORMAL, LOW);
  tone(BUZZER_PIN, NADA_ALARM_HZ);
}

// Evaluasi multi-kondisi dan cetak pesan pembeda per pemicu
void evaluasiKondisi(float suhu, float kelembaban) {
  bool suhuKritis        = (suhu > AMBANG_SUHU_KRITIS);
  bool kelembabanRendah  = (kelembaban < AMBANG_KELEMBABAN_RENDAH);

  if (suhuKritis || kelembabanRendah) {
    setStatusAlarm();
    if (suhuKritis && kelembabanRendah) {
      Serial.println(">>> PERINGATAN [SUHU & KELEMBABAN]: Suhu terlalu tinggi DAN udara terlalu kering! <<<");
    } else if (suhuKritis) {
      Serial.println(">>> PERINGATAN [SUHU]: Suhu melebihi batas kritis (> 38 C)! <<<");
    } else {
      Serial.println(">>> PERINGATAN [KELEMBABAN]: Udara terlalu kering (< 20 %)! <<<");
    }
  } else {
    setStatusNormal();
    Serial.println("Status: Lingkungan Normal & Terkendali.");
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  Serial.println("--- Inisialisasi Node IoT ESP32 (Stand-alone Mode) ---");

  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  sambungkanWiFi();
}

// ---------- Loop Utama ----------
void loop() {
  unsigned long waktuSekarang = millis();

  if (waktuSekarang - waktuSebelumnya >= intervalBaca) {
    waktuSebelumnya = waktuSekarang;

    TempAndHumidity data = dhtSensor.getTempAndHumidity();

    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println("[Error] Gagal membaca data dari sensor DHT22!");
      return;
    }

    int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : RSSI_MOCK;

    Serial.printf("[Sensor Data] Suhu: %.2f C | Kelembaban: %.2f %% | RSSI: %d dBm\n",
                  data.temperature, data.humidity, rssi);

    evaluasiKondisi(data.temperature, data.humidity);
  }

  // Task background lain dapat berjalan di sini tanpa terhalang delay()
}
