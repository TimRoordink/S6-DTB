#include <WiFi.h>
#include <WebServer.h>
#include <sqlite3.h>
#include "SPIFFS.h"
#include "html.h"
#include <esp32/rom/rtc.h>
#include "driver/adc.h"
#include <time.h>


// #define DROP_ALL_AT_START // Only uncomment when table structure changed

const char* ssid = "HotspotEmiel";
const char* password = "HotspotEmiel";

int roomID = 1;
unsigned long lastSensorRead = 0;
const unsigned long interval = 5000;

WebServer server(80);
sqlite3* db;

// === Callback en SQL helper ===
int callback(void* NotUsed, int argc, char** argv, char** azColName) {
  for (int i = 0; i < argc; i++) {
    Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  return 0;
}

void checkOK(int rc, char* zErrMsg, const char* sql) {
  if (rc != SQLITE_OK) {
    Serial.printf("FOUT: %s\nQuery: %s\n", zErrMsg, sql);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("OK: %s\n", sql);
  }
}

int executeSQL(const char* sql) {
  char* zErrMsg = 0;
  int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
  checkOK(rc, zErrMsg, sql);
  return rc;
}

void handleRoot() {
  String page = htmlPage;

  server.send(200, "text/html", page);
}

String exportTableCSV(const char* tableName) {
  String result = "";
  sqlite3_stmt* stmt;
  String query = "SELECT * FROM ";
  query += tableName;

  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
    Serial.printf("SQL prepare error: %s\n", sqlite3_errmsg(db));
    return "ERROR: Cannot prepare query.\n";
  }

  // Column headers
  int cols = sqlite3_column_count(stmt);
  for (int i = 0; i < cols; i++) {
    result += sqlite3_column_name(stmt, i);
    if (i < cols - 1) result += ",";
  }
  result += "\n";

  // Rows
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    for (int i = 0; i < cols; i++) {
      const char* val = (const char*)sqlite3_column_text(stmt, i);
      result += val ? val : "NULL";
      if (i < cols - 1) result += ",";
    }
    result += "\n";
  }

  sqlite3_finalize(stmt);
  return result;
}

void handleGetRooms() {
  String json = "[";
  sqlite3_stmt* stmt;
  const char* query = "SELECT id, name, floor, owner FROM rooms";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) == SQLITE_OK) {
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      if (!first) json += ",";
      json += "{";
      json += "\"id\":" + String(sqlite3_column_int(stmt, 0)) + ",";
      json += "\"name\":\"" + String((const char*)sqlite3_column_text(stmt, 1)) + "\",";
      json += "\"floor\":" + String(sqlite3_column_int(stmt, 2)) + ",";
      json += "\"owner\":\"" + String((const char*)sqlite3_column_text(stmt, 3)) + "\"";
      json += "}";
      first = false;
    }
    sqlite3_finalize(stmt);
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleUpdateRoom() {
  String id = server.arg("id");
  String name = server.arg("name");
  String floor = server.arg("floor");
  String owner = server.arg("owner");
  String sql = "UPDATE rooms SET name='" + name + "', floor=" + floor + ", owner='" + owner + "' WHERE id=" + id + ";";
  executeSQL(sql.c_str());
  server.send(200);
}

void handleDeleteRoom() {
  String id = server.arg("id");
  String sql = "DELETE FROM rooms WHERE id=" + id + ";";
  executeSQL(sql.c_str());
  server.send(200);
}

void handleAddRoom() {
  String name = server.arg("name");
  String floor = server.arg("floor");
  String owner = server.arg("owner");
  String sql = "INSERT INTO rooms (name, floor, owner) VALUES ('" + name + "', " + floor + ", '" + owner + "');";
  executeSQL(sql.c_str());
  server.send(200);
}

void handleClearRooms() {
  executeSQL("DELETE FROM rooms;");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRefreshData() {
  String html = "<table><tr>"
                "<th>Timestamp</th><th>Status</th><th>Temperature</th><th>Hall</th><th>Room</th></tr>";

  const char* sql = R"sql(
    SELECT d.timestamp, s.description, d.temperature, d.hall, r.name
    FROM samples d
    LEFT JOIN status s ON d.status = s.id
    LEFT JOIN rooms r ON d.room_id = r.id
    ORDER BY d.timestamp DESC
    LIMIT 50
  )sql";
 
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to prepare refreshData query: %s\n", sqlite3_errmsg(db));
    server.send(500, "text/plain", "Database query error.");
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* timestamp = (const char*)sqlite3_column_text(stmt, 0);
    const char* statusDesc = (const char*)sqlite3_column_text(stmt, 1);
    double temperature = sqlite3_column_double(stmt, 2);
    double hall = sqlite3_column_double(stmt, 3);
    const char* roomName = (const char*)sqlite3_column_text(stmt, 4);

    html += "<tr>";
    html += "<td>" + String(timestamp ? timestamp : "N/A") + "</td>";
    html += "<td>" + String(statusDesc ? statusDesc : "N/A") + "</td>";
    html += "<td>" + String(temperature, 2) + "</td>";
    html += "<td>" + String(hall, 2) + "</td>";
    html += "<td>" + String(roomName ? roomName : "N/A") + "</td>";
    html += "</tr>";
  }

  sqlite3_finalize(stmt);
  html += "</table>";

  server.send(200, "text/html", html);
}

void setupTime() {
  // Set timezone: CET/CEST (Amsterdam, with daylight saving)
  setenv("TZ", "CEST-1CET,M3.2.0/2:00:00,M11.1.0/2:00:00", 1);
  tzset();  // Apply the timezone setting

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Wachten op NTP tijd");
  time_t now = time(nullptr);
  int retry = 0;
  const int retry_limit = 10;
  while (now < 8 * 3600 * 2 && retry < retry_limit) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retry++;
  }
  Serial.println();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fout bij ophalen tijd");
    return;
  }
  Serial.println("Tijd gesynchroniseerd:");
  Serial.println(asctime(&timeinfo));
}



void setupHall() {
  // Configure ADC channels
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);  // GPIO36
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);  // GPIO39
}

int hallReadDIY() {
  int ch0 = adc1_get_raw(ADC1_CHANNEL_0);  // GPIO36
  int ch3 = adc1_get_raw(ADC1_CHANNEL_3);  // GPIO39

  return (ch0 - ch3) / 2;
}

void insertSensorData(float temperature, int hallValue) {
  // Get current time as ISO8601 string
  time_t nowTime = time(nullptr);
  struct tm* nowTm = localtime(&nowTime);

  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", nowTm);
  int status = random(1, 5);
  // Compose SQL query string
  char sql[256];
  snprintf(sql, sizeof(sql),
           "INSERT OR REPLACE INTO samples (timestamp, status, temperature, room_id, hall) "
           "VALUES ('%s', %d, %.2f, %d, %d);",
           timestamp,
           status,  // status id, change if you have dynamic statuses
           temperature,
           roomID,
           hallValue);

  executeSQL(sql);
}

void setup() {
  Serial.begin(115200);
  setupHall();
  WiFi.begin(ssid, password);
  Serial.print("Verbinden met WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbonden: " + WiFi.localIP().toString());
  setupTime();

  // if (!SPIFFS.format()) {
  //   Serial.println("SPIFFS format mislukt");
  // }

  // Mount SPIFFS en open database
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mislukt");
    return;
  }

  int rc = sqlite3_open("/spiffs/test.db", &db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(db));
    return;
  }

  // Tabelstructuur
#ifdef DROP_ALL_AT_START
  executeSQL("DROP TABLE IF EXISTS samples;");
  executeSQL("DROP TABLE IF EXISTS rooms;");
  executeSQL("DROP TABLE IF EXISTS status;");
  delay(1000);
#endif
  executeSQL("CREATE TABLE IF NOT EXISTS samples (timestamp TEXT PRIMARY KEY, status INT, temperature REAL, room_id INT, hall REAL);");
  executeSQL("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY, name TEXT, floor INT, owner TEXT);");
  executeSQL("CREATE TABLE IF NOT EXISTS status (id INTEGER PRIMARY KEY, description TEXT UNIQUE);");

  // Insert statuses with fixed IDs, only if they don't already exist
  executeSQL("INSERT OR IGNORE INTO status (id, description) VALUES (1, 'Temp High');");
  executeSQL("INSERT OR IGNORE INTO status (id, description) VALUES (2, 'Temp Low');");
  executeSQL("INSERT OR IGNORE INTO status (id, description) VALUES (3, 'OK');");
  executeSQL("INSERT OR IGNORE INTO status (id, description) VALUES (4, 'Hall Error');");

  Serial.println("Schema check:");
  executeSQL("SELECT sql FROM sqlite_master WHERE type='table';");

  // Webserver-routes
  server.on("/", handleRoot);
  server.on("/getRooms", handleGetRooms);
  server.on("/updateRoom", HTTP_POST, handleUpdateRoom);
  server.on("/deleteRoom", HTTP_POST, handleDeleteRoom);
  server.on("/addRoom", HTTP_POST, handleAddRoom);
  server.on("/clearRooms", HTTP_POST, handleClearRooms);
  server.on("/refreshSamples", handleRefreshData);

  server.begin();
  Serial.println("Webserver actief.");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (now - lastSensorRead >= interval) {
    lastSensorRead = now;

    // Read sensors here
    float temperature = temperatureRead();
    int hallValue = hallReadDIY();
    insertSensorData(temperature, hallValue);

    // Optionally, insert into your database or update variables here
  }
}
