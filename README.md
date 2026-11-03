# Praktikum IoT Pertemuan 6 - Embedded System Simulator (ESP32 + DHT22)

Node IoT berbasis ESP32 yang disimulasikan di [Wokwi](https://wokwi.com): sensor suhu/kelembaban DHT22, LED hijau (normal), LED merah (alarm), dan piezo buzzer.

## Isi repository

| Folder | Keterangan |
|---|---|
| `multi_condition_alert/` | Firmware utama: pembacaan non-blocking `millis()` + alarm jika suhu > 38 °C **atau** kelembaban < 20 % |
| `deep_sleep_node/` | Contoh firmware deep sleep: bangun tiap 10 menit, baca, kirim, tidur |

## Pengkabelan

| Komponen | Pin ESP32 |
|---|---|
| DHT22 VCC / Data / GND | 3V3 / GPIO 15 / GND |
| LED hijau (via R 220 Ω) | GPIO 2 |
| LED merah (via R 220 Ω) | GPIO 4 |
| Buzzer (+) | GPIO 5 |

## Cara menjalankan di Wokwi

1. Buka wokwi.com, buat project **ESP32**.
2. Tambahkan DHT22, 2 LED, 2 resistor 220 Ω, dan buzzer sesuai tabel di atas.
3. Tab *Library Manager*: tambahkan `DHT sensor library for ESPx`.
4. Salin isi file `.ino` ke `sketch.ino`, lalu klik **Start Simulation**.
5. Klik DHT22 dan geser slider suhu/kelembaban untuk menguji alarm.

## Link project

- Wokwi: _[https://wokwi.com/projects/479096966031898625]_

## Penulis

- _Hakim_Syarifudin/24063005_ - Informatika UNISDA
