#include <WiFi.h>
#include <WebServer.h>
#include <sqlite3.h>
#include "SPIFFS.h"
#include "html.h"

#define DROP_ALL_AT_START // Only uncomment when table structure changed

const char* ssid = "Marco34";
const char* password = "Snijheesters34";

int roomID = 1;

WebServer server(80);
sqlite3 *db;

// === Callback en SQL helper ===
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
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

void handleAddRoom() {
  String name = server.arg("roomname");
  // int floor = server.arg("floor");
  // String owner = server.arg("owner");
  String sql = "INSERT INTO rooms (name) VALUES ('" + name + "');";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleViewRooms() {
  String data = exportTableCSV("rooms");
  server.send(200, "text/plain", data);
}

void handleDeleteRoom() {
  String id = server.arg("room_id");
  executeSQL(("DELETE FROM rooms WHERE id = " + id + ";").c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearRooms() {
  executeSQL("DELETE FROM rooms;");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRefreshSamples(){

}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Verbinden met WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi verbonden: " + WiFi.localIP().toString());

  // Mount SPIFFS en open database
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mislukt");
    return;
  }

  int rc = sqlite3_open("/spiffs/test.db", &db);
  if (rc) {
    Serial.println("Database kon niet worden geopend!");
    return;
  }

  // Tabelstructuur
#ifdef DROP_ALL_AT_START
  executeSQL("DROP TABLE IF EXISTS users;");
  executeSQL("DROP TABLE IF EXISTS rooms;");
  executeSQL("DROP TABLE IF EXISTS devices;");
  executeSQL("DROP TABLE IF EXISTS user_room;");
  executeSQL("DROP TABLE IF EXISTS device_room;");
  executeSQL("DROP TABLE IF EXISTS samples");
  executeSQL("DROP TABLE IF EXISTS status");
#endif
  executeSQL("CREATE TABLE IF NOT EXISTS samples (timestamp TEXT PRIMARY KEY, status INT, temperature REAL, hall REAL);");
  executeSQL("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY, name TEXT, floor INT, owner TEXT);");
  executeSQL("CREATE TABLE IF NOT EXISTS status (id INTEGER PRIMARY KEY, discription TEXT);");


  // Webserver-routes
  server.on("/", handleRoot);
  server.on("/addRoom", HTTP_POST, handleAddRoom);
  server.on("/viewRooms", handleViewRooms);
  server.on("/deleteRoom", HTTP_POST, handleDeleteRoom);
  server.on("/clearRooms", HTTP_POST, handleClearRooms);
  server.on("/refreshSamples", handleRefreshSamples);

  server.begin();
  Serial.println("Webserver actief.");
}

void loop() {
  server.handleClient();
}
