#include <WiFi.h>
#include <WebServer.h>
#include <sqlite3.h>
#include "SPIFFS.h"
#include "html.h"

#define DROP_ALL_AT_START // Only uncomment when table structure changed

const char* ssid = "HotspotEmiel";
const char* password = "HotspotEmiel";

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

  String userOptions = generateOptions("users");
  String roomOptions = generateOptions("rooms");
  String deviceOptions = generateOptions("devices");

  String userRoomOptions = generateCouplingOptions("user_room", "Gebruiker ID", "Kamer ID");
  String deviceRoomOptions = generateCouplingOptions("device_room", "Apparaat ID", "Kamer ID");

  page.replace("%USER_OPTIONS%", userOptions);
  page.replace("%ROOM_OPTIONS%", roomOptions);
  page.replace("%DEVICE_OPTIONS%", deviceOptions);

  page.replace("%USER_ROOM_OPTIONS%", userRoomOptions);
  page.replace("%DEVICE_ROOM_OPTIONS%", deviceRoomOptions);

  server.send(200, "text/html", page);
}

void handleAddUser() {
  String name = server.arg("username");
  String bdate = server.arg("b_date");
  String sql = "INSERT INTO users (name, bdate) VALUES ('" + name + "', '" + bdate + "');";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAddRoom() {
  String name = server.arg("roomname");
  String sql = "INSERT INTO rooms (name) VALUES ('" + name + "');";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAddDevice() {
  String name = server.arg("devicename");
  String sql = "INSERT INTO devices (name) VALUES ('" + name + "');";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLinkUserRoom() {
  String uid = server.arg("user_id");
  String rid = server.arg("room_id");
  String sql = "INSERT INTO user_room (user_id, room_id) VALUES (" + uid + ", " + rid + ");";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLinkDeviceRoom() {
  String did = server.arg("device_id");
  String rid = server.arg("room_id");
  String sql = "INSERT INTO device_room (device_id, room_id) VALUES (" + did + ", " + rid + ");";
  executeSQL(sql.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
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

void handleDownloadUsers() {
  String data = exportTableCSV("users");
  server.sendHeader("Content-Disposition", "attachment; filename=users.csv");
  server.send(200, "text/csv", data);
}

void handleDownloadRooms() {
  String data = exportTableCSV("rooms");
  server.sendHeader("Content-Disposition", "attachment; filename=rooms.csv");
  server.send(200, "text/csv", data);
}

void handleDownloadDevices() {
  String data = exportTableCSV("devices");
  server.sendHeader("Content-Disposition", "attachment; filename=devices.csv");
  server.send(200, "text/csv", data);
}

void handleDownloadUserRoom() {
  String data = exportTableCSV("user_room");
  server.sendHeader("Content-Disposition", "attachment; filename=user_room.csv");
  server.send(200, "text/csv", data);
}

void handleDownloadDeviceRoom() {
  String data = exportTableCSV("device_room");
  server.sendHeader("Content-Disposition", "attachment; filename=device_room.csv");
  server.send(200, "text/csv", data);
}

void handleViewUsers() {
  String data = exportTableCSV("users");
  server.send(200, "text/plain", data);
}

void handleViewRooms() {
  String data = exportTableCSV("rooms");
  server.send(200, "text/plain", data);
}

void handleViewDevices() {
  String data = exportTableCSV("devices");
  server.send(200, "text/plain", data);
}

void handleViewUserRoom() {
  String data = exportTableCSV("user_room");
  server.send(200, "text/plain", data);
}

void handleViewDeviceRoom() {
  String data = exportTableCSV("device_room");
  server.send(200, "text/plain", data);
}

void handleDeleteUser() {
  String id = server.arg("user_id");
  executeSQL(("DELETE FROM user_room WHERE user_id = " + id + ";").c_str());
  executeSQL(("DELETE FROM users WHERE id = " + id + ";").c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDeleteRoom() {
  String id = server.arg("room_id");
  executeSQL(("DELETE FROM user_room WHERE room_id = " + id + ";").c_str());
  executeSQL(("DELETE FROM device_room WHERE room_id = " + id + ";").c_str());
  executeSQL(("DELETE FROM rooms WHERE id = " + id + ";").c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDeleteDevice() {
  String id = server.arg("device_id");
  executeSQL(("DELETE FROM device_room WHERE device_id = " + id + ";").c_str());
  executeSQL(("DELETE FROM devices WHERE id = " + id + ";").c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearUsers() {
  executeSQL("DELETE FROM user_room;");
  executeSQL("DELETE FROM users;");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearRooms() {
  executeSQL("DELETE FROM user_room;");
  executeSQL("DELETE FROM device_room;");
  executeSQL("DELETE FROM rooms;");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearDevices() {
  executeSQL("DELETE FROM device_room;");
  executeSQL("DELETE FROM devices;");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDeleteUserRoom() {
  String coupling = server.arg("user_room_coupling"); // e.g., "3,5"
  int commaIndex = coupling.indexOf(',');
  if (commaIndex == -1) {
    server.send(400, "text/plain", "Invalid input");
    return;
  }

  int user_id = coupling.substring(0, commaIndex).toInt();
  int room_id = coupling.substring(commaIndex + 1).toInt();

  String sql = "DELETE FROM user_room WHERE user_id = ? AND room_id = ?";
  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleDeleteDeviceRoom() {
  String coupling = server.arg("device_room_coupling"); // e.g., "2,4"
  int commaIndex = coupling.indexOf(',');
  if (commaIndex == -1) {
    server.send(400, "text/plain", "Invalid input");
    return;
  }

  int device_id = coupling.substring(0, commaIndex).toInt();
  int room_id = coupling.substring(commaIndex + 1).toInt();

  String sql = "DELETE FROM device_room WHERE device_id = ? AND room_id = ?";
  sqlite3_stmt* stmt;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, device_id);
    sqlite3_bind_int(stmt, 2, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleClearUserRoom() {
  String sql = "DELETE FROM user_room";
  char* errMsg = nullptr;
  sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleClearDeviceRoom() {
  String sql = "DELETE FROM device_room";
  char* errMsg = nullptr;
  sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

String generateOptions(const char* tableName) {
  String options = "";
  String sql = "SELECT id, name FROM ";
  sql += tableName;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int id = sqlite3_column_int(stmt, 0);
      const unsigned char* name = sqlite3_column_text(stmt, 1);
      options += "<option value='" + String(id) + "'>" + String(id) + " - " + String((const char*)name) + "</option>";
    }
    sqlite3_finalize(stmt);
  } else {
    options = "<option>Error loading " + String(tableName) + "</option>";
  }

  return options;
}

String generateCouplingOptions(const char* tableName, const char* leftLabel, const char* rightLabel) {
  String options = "";
  String sql = "SELECT * FROM ";
  sql += tableName;

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int left = sqlite3_column_int(stmt, 0);
      int right = sqlite3_column_int(stmt, 1);
      String value = String(left) + "," + String(right); // value passed in form
      String label = leftLabel + String(" ") + String(left) + " - " + rightLabel + String(" ") + String(right);
      options += "<option value='" + value + "'>" + label + "</option>";
    }
    sqlite3_finalize(stmt);
  } else {
    options = "<option>Error loading " + String(tableName) + "</option>";
  }

  return options;
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
#endif
  executeSQL("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, bdate TEXT);");
  executeSQL("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY, name TEXT);");
  executeSQL("CREATE TABLE IF NOT EXISTS devices (id INTEGER PRIMARY KEY, name TEXT);");
  executeSQL("CREATE TABLE IF NOT EXISTS user_room (user_id INTEGER, room_id INTEGER);");
  executeSQL("CREATE TABLE IF NOT EXISTS device_room (device_id INTEGER, room_id INTEGER);");

  // Webserver-routes
  server.on("/", handleRoot);
  server.on("/addUser", HTTP_POST, handleAddUser);
  server.on("/addRoom", HTTP_POST, handleAddRoom);
  server.on("/addDevice", HTTP_POST, handleAddDevice);
  server.on("/linkUserRoom", HTTP_POST, handleLinkUserRoom);
  server.on("/linkDeviceRoom", HTTP_POST, handleLinkDeviceRoom);
  server.on("/downloadUsers", handleDownloadUsers);
  server.on("/downloadRooms", handleDownloadRooms);
  server.on("/downloadDevices", handleDownloadDevices);
  server.on("/downloadUserRoom", handleDownloadUserRoom);
  server.on("/downloadDeviceRoom", handleDownloadDeviceRoom);
  server.on("/viewUsers", handleViewUsers);
  server.on("/viewRooms", handleViewRooms);
  server.on("/viewDevices", handleViewDevices);
  server.on("/viewUserRoom", handleViewUserRoom);
  server.on("/viewDeviceRoom", handleViewDeviceRoom);
  server.on("/deleteUser", HTTP_POST, handleDeleteUser);
  server.on("/deleteRoom", HTTP_POST, handleDeleteRoom);
  server.on("/deleteDevice", HTTP_POST, handleDeleteDevice);
  server.on("/clearUsers", HTTP_POST, handleClearUsers);
  server.on("/clearRooms", HTTP_POST, handleClearRooms);
  server.on("/clearDevices", HTTP_POST, handleClearDevices);
  server.on("/deleteUserRoom", HTTP_POST, handleDeleteUserRoom);
  server.on("/deleteDeviceRoom", HTTP_POST, handleDeleteDeviceRoom);
  server.on("/clearUserRoom", HTTP_POST, handleClearUserRoom);
  server.on("/clearDeviceRoom", HTTP_POST, handleClearDeviceRoom);

  server.begin();
  Serial.println("Webserver actief.");
}

void loop() {
  server.handleClient();
}
