#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Konfigurasi Wi-Fi Access Point (Bisa diubah sesuai keinginan)
const char* ssid = "Antrean_Pintar_Kelompok3";
const char* password = "irpanjagonyolder";

// Membuat objek web server pada port 80
AsyncWebServer server(80);

// --- PEMETAAN PIN GPIO ESP32-S3 ---
//Pin tambah digit untuk IC 74HC192 Satuan
const int pinNextWeb = 6;

// Pin Digit Satuan (Dari IC 74HC192 ke-1)
const int pinS0 = 10;
const int pinS1 = 11;
const int pinS2 = 12;
const int pinS3 = 13;

// Pin Digit Puluhan (Dari IC 74HC192 ke-2)
const int pinP0 = 14;
const int pinP1 = 15;
const int pinP2 = 16;
const int pinP3 = 17;

// Pin Output untuk Fitur Reset Otomatis
const int pinReset = 18;

// --- HALAMAN HTML, CSS & JAVASCRIPT ---
// Menggunakan raw string literal (R"rawliteral(...)rawliteral") agar mudah ditulis
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Sistem Antrian - Kelompok 3</title>
  <style>
    /* Mereset margin bawaan browser */
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }

    /* Mengatur latar belakang halaman agar full screen dan berada di tengah */
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
      /* Latar belakang gradien warna modern */
      background: linear-gradient(135deg, #667eea 0%, #d44db5 100%); 
      height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    /* Membuat kotak kartu putih bergaya modern */
    .card {
      background-color: #ffffff;
      padding: 40px 30px;
      border-radius: 20px;
      box-shadow: 0 15px 30px rgba(0,0,0,0.2);
      text-align: center;
      width: 90%;
      max-width: 400px;
    }

    h1 {
      color: #333;
      font-size: 28px;
      margin-bottom: 5px;
    }

    h2 {
      color: #666;
      font-size: 18px;
      font-weight: normal;
      margin-bottom: 20px;
    }

    /* Mempercantik tampilan angka agar mirip layar digital */
    .angka-antrian { 
      font-size: 120px; 
      font-weight: bold; 
      color: #2c3e50;
      background-color: #f8f9fa;
      border: 2px dashed #cbd5e1;
      border-radius: 15px;
      margin: 20px 0;
      padding: 10px 0;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
    }

    /* Mengatur posisi tombol agar sejajar dan rapi */
    .btn-container {
      display: flex;
      justify-content: space-between;
      gap: 15px;
    }

    .btn { 
      flex: 1; /* Membuat ukuran tombol seimbang */
      padding: 15px 0; 
      font-size: 18px; 
      font-weight: bold;
      cursor: pointer; 
      border: none; 
      border-radius: 10px;
      transition: all 0.3s ease; /* Animasi transisi yang mulus */
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }

    .btn-next { 
      background-color: #4CAF50; 
      color: white; 
    }

    /* Efek saat tombol Selanjutnya disorot mouse */
    .btn-next:hover {
      background-color: #45a049;
      transform: translateY(-3px); /* Tombol seolah terangkat */
      box-shadow: 0 6px 12px rgba(76, 175, 80, 0.3);
    }

    .btn-reset { 
      background-color: #f44336; 
      color: white; 
    }

    /* Efek saat tombol Reset disorot mouse */
    .btn-reset:hover {
      background-color: #da190b;
      transform: translateY(-3px);
      box-shadow: 0 6px 12px rgba(244, 67, 54, 0.3);
    }
  </style>
</head>
<body>

  <div class="card">
    <h1>Nomor Antrian</h1>
    <h2>Kelompok 3</h2>
    
    <div class="angka-antrian" id="tampilAngka">00</div>

    <div class="btn-container">
      <button class="btn btn-next" onclick="tambahAntrian()">Selanjutnya</button>
      <button class="btn btn-reset" onclick="resetAntrian()">Reset</button>
    </div>
  </div>

  <script>
    // 1. FUNGSI AKTIF: Mengambil data secara real-time dari ESP32 setiap 500 milidetik
    setInterval(function() {
      fetch('/data') // Endpoint disesuaikan dengan program backend ESP32
        .then(response => response.text())
        .then(data => {
          // Format angka agar selalu 2 digit (misal: "7" diubah menjadi "07")
          let angkaDuaDigit = data.padStart(2, '0');
          document.getElementById("tampilAngka").innerText = angkaDuaDigit;
        })
        .catch(error => {
          console.log("Gagal sinkronisasi data dengan ESP32...");
        });
    }, 500); 
    

    // 2. FUNGSI TOMBOL SELANJUTNYA
    function tambahAntrian() {
      fetch('/next') // Mengaktifkan kembali sinyal ke ESP32
        .then(response => {
          console.log("Sinyal NEXT berhasil dikirim ke IC Counter via ESP32");
        })
        .catch(error => {
          alert("Gagal menambah antrean, periksa koneksi Wi-Fi!");
        });
    }

    // 3. FUNGSI TOMBOL RESET (KONTROL HARDWARE DARI SOFTWARE)
    function resetAntrian() {
      let konfirmasi = confirm("Yakin ingin mereset antrian ke 00?");
      if (konfirmasi) {
        fetch('/reset') // Mengaktifkan sinyal HTTP GET ke ESP32 untuk memicu GPIO 18
          .then(response => {
            console.log("Sinyal reset berhasil dikirim ke IC Counter via ESP32");
          })
          .catch(error => {
            alert("Gagal mereset, periksa koneksi Wi-Fi ke ESP32!");
          });
      }
    }
  </script>

</body>
</html>
)rawliteral";

// --- FUNGSI PEMBACAAN HARDWARE ---
int getQueueNumber() {
  // Membaca logika biner dari IC Satuan dan mengubahnya ke desimal
  int satuan = (digitalRead(pinS0) * 1) + 
               (digitalRead(pinS1) * 2) + 
               (digitalRead(pinS2) * 4) + 
               (digitalRead(pinS3) * 8);

  // Membaca logika biner dari IC Puluhan dan mengubahnya ke desimal
  int puluhan = (digitalRead(pinP0) * 1) + 
                (digitalRead(pinP1) * 2) + 
                (digitalRead(pinP2) * 4) + 
                (digitalRead(pinP3) * 8);

  // Menggabungkan menjadi 1 angka utuh (0 - 99)
  return (puluhan * 10) + satuan;
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi Mode Pin
  pinMode(pinS0, INPUT); pinMode(pinS1, INPUT); pinMode(pinS2, INPUT); pinMode(pinS3, INPUT);
  pinMode(pinP0, INPUT); pinMode(pinP1, INPUT); pinMode(pinP2, INPUT); pinMode(pinP3, INPUT);
  
  pinMode(pinReset, OUTPUT);
  digitalWrite(pinReset, LOW); // Pastikan kondisi awal LOW agar IC bisa menghitung

  // Inisialisasi Mode Pin 6 untuk tambah dari ESP ke 74HC192
  pinMode(pinNextWeb, OUTPUT);
  digitalWrite(pinNextWeb, HIGH); // Standar input IC 74HC192 adalah HIGH (Active Low)

  // Memulai Wi-Fi Mode Access Point
  Serial.println("Memulai Access Point...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Web Server aktif! Sambungkan HP/Laptop ke Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Lalu buka browser dan ketik alamat IP ini: ");
  Serial.println(IP);

  // --- ROUTING WEB SERVER ---
  
  // 1. Jika pengguna membuka halaman utama (Root)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // 2. Endpoint untuk memberikan data angka antrean saat diminta oleh JavaScript
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    int currentQueue = getQueueNumber();
    request->send(200, "text/plain", String(currentQueue));
  });

  // 3. Endpoint saat tombol Reset ditekan di Web
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    // Kirim pulsa HIGH sesaat ke pin 14 IC 74HC192
    digitalWrite(pinReset, HIGH);
    delay(50); // Tahan 50 milidetik
    digitalWrite(pinReset, LOW);
    
    request->send(200, "text/plain", "OK");
  });

  // 4. Endpoint saat tombol Selanjutnya ditekan di Web
  server.on("/next", HTTP_GET, [](AsyncWebServerRequest *request){
    // Membuat pulsa LOW sesaat (mensimulasikan jatuhnya pulsa dari NE555)
    digitalWrite(pinNextWeb, LOW);
    delay(50); // Tahan 50 milidetik
    digitalWrite(pinNextWeb, HIGH);

    request->send(200, "text/plain", "OK");
  });

  // Mulai Server
  server.begin();
}

void loop() {
  // Kosong. Semua proses web berjalan asinkronus di latar belakang.
  // ESP32 Anda tidak akan mengalami lag.
}