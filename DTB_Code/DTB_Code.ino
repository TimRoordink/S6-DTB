// Include necessary libraries
#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <sqlite3.h>

// Replace with your network credentials
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Create server on port 80
AsyncWebServer server(80);

// SQLite database object
SQLiteDB db;

// Current selected room for sampling
int currentRoom = -1;

// Sampling interval in milliseconds (e.g., 60 seconds)
const unsigned long sampleInterval = 60000;
unsigned long lastSample = 0;

// Forward declarations
void setupDatabase();
void setupWebServer();
void insertSample();

void setup() {
  Serial.begin(115200);
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Initialize database
  setupDatabase();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  setupWebServer();
  server.begin();
}

void loop() {
  unsigned long now = millis();
  if (currentRoom > 0 && (now - lastSample >= sampleInterval)) {
    insertSample();
    lastSample = now;
  }
}

void setupDatabase() {
  // Open or create database in SPIFFS
  if (!db.open("/spiffs/app.db")) {
    Serial.println("Failed to open database");
    return;
  }
  // Create tables in 3NF
  db.exec("CREATE TABLE IF NOT EXISTS person ("  
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "name TEXT NOT NULL);");
  db.exec("CREATE TABLE IF NOT EXISTS room ("  
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "name TEXT NOT NULL, "
          "owner_id INTEGER NOT NULL, "
          "FOREIGN KEY(owner_id) REFERENCES person(id));");
  db.exec("CREATE TABLE IF NOT EXISTS sample ("  
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "room_id INTEGER NOT NULL, "
          "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
          "temperature REAL, "
          "hall REAL, "
          "FOREIGN KEY(room_id) REFERENCES room(id));");
}

void setupWebServer() {
  // Home page with navigation
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<h1>ESP32 Data Logger</h1>"
                  "<ul>"
                  "<li><a href=\"/persons\">Manage Persons</a></li>"
                  "<li><a href=\"/rooms\">Manage Rooms</a></li>"
                  "<li><a href=\"/settings\">Settings</a></li>"
                  "<li><a href=\"/samples\">View Samples</a></li>"
                  "</ul>";
    request->send(200, "text/html", html);
  });

  // List and add persons
  server.on("/persons", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Query persons
    SQLiteQuery q = db.query("SELECT id, name FROM person;");
    String html = "<h2>Persons</h2><ul>";
    while (q.next()) {
      html += "<li>" + String(q.getInt(0)) + ": " + q.getString(1) + "</li>";
    }
    html += "</ul>";
    html += "<h3>Add Person</h3>";
    html += "<form action=\"/add_person\" method=\"POST\">";
    html += "Name: <input name=\"name\" required><input type=submit value=\"Add\">";
    html += "</form><p><a href=\"/\">Back</a></p>";
    request->send(200, "text/html", html);
  });

  server.on("/add_person", HTTP_POST, [](AsyncWebServerRequest *request) {
    String name = request->arg("name");
    SQLiteStatement stmt = db.prepare("INSERT INTO person (name) VALUES (?);");
    stmt.bind(1, name);
    stmt.exec();
    request->redirect("/persons");
  });

  // List and add rooms
  server.on("/rooms", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Query rooms with owner name
    SQLiteQuery q = db.query(""  
      "SELECT r.id, r.name, p.name "
      "FROM room r JOIN person p ON r.owner_id = p.id;");
    String html = "<h2>Rooms</h2><ul>";
    while (q.next()) {
      html += "<li>" + String(q.getInt(0)) + ": " + q.getString(1) + " (Owner: " + q.getString(2) + ")</li>";
    }
    html += "</ul>";
    // Fetch persons for dropdown
    SQLiteQuery p = db.query("SELECT id, name FROM person;");
    html += "<h3>Add Room</h3><form action=\"/add_room\" method=\"POST\">";
    html += "Name: <input name=\"name\" required><br>Owner: <select name=\"owner_id\">";
    while (p.next()) {
      html += "<option value='" + String(p.getInt(0)) + "'>" + p.getString(1) + "</option>";
    }
    html += "</select><br><input type=submit value=\"Add\"></form>";
    html += "<p><a href=\"/\">Back</a></p>";
    request->send(200, "text/html", html);
  });

  server.on("/add_room", HTTP_POST, [](AsyncWebServerRequest *request) {
    String name = request->arg("name");
    int ownerId = request->arg("owner_id").toInt();
    SQLiteStatement stmt = db.prepare("INSERT INTO room (name, owner_id) VALUES (?, ?);");
    stmt.bind(1, name);
    stmt.bind(2, ownerId);
    stmt.exec();
    request->redirect("/rooms");
  });

  // Settings: select current room
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Fetch rooms
    SQLiteQuery q = db.query("SELECT id, name FROM room;");
    String html = "<h2>Settings</h2><form action=\"/set_settings\" method=\"POST\">";
    html += "Select Room Context: <select name=\"room_id\">";
    while (q.next()) {
      int id = q.getInt(0);
      html += "<option value='" + String(id) + "'" + (id==currentRoom?" selected":"") + ">" + q.getString(1) + "</option>";
    }
    html += "</select><br><input type=submit value=\"Save\"></form>";
    html += "<p><a href=\"/\">Back</a></p>";
    request->send(200, "text/html", html);
  });
  server.on("/set_settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    currentRoom = request->arg("room_id").toInt();
    request->redirect("/settings");
  });

  // View samples
  server.on("/samples", HTTP_GET, [](AsyncWebServerRequest *request) {
    SQLiteQuery q = db.query("SELECT s.id, s.timestamp, s.temperature, s.hall, r.name "
                              "FROM sample s JOIN room r ON s.room_id = r.id "
                              "ORDER BY s.timestamp DESC LIMIT 50;");
    String html = "<h2>Last Samples</h2><table border=1><tr><th>ID</th><th>Time</th><th>Temp</th><th>Hall</th><th>Room</th></tr>";
    while(q.next()) {
      html += "<tr><td>" + String(q.getInt(0)) + "</td>";
      html += "<td>" + q.getString(1) + "</td>";
      html += "<td>" + String(q.getFloat(2),2) + "</td>";
      html += "<td>" + String(q.getFloat(3),2) + "</td>";
      html += "<td>" + q.getString(4) + "</td></tr>";
    }
    html += "</table><p><a href=\"/\">Back</a></p>";
    request->send(200, "text/html", html);
  });
}

void insertSample() {
  // Read internal sensors
  float temp = temperatureRead();    // internal temperature sensor
  float hall = hallRead();           // internal hall sensor
  
  // Insert into database
  SQLiteStatement stmt = db.prepare("INSERT INTO sample (room_id, temperature, hall) VALUES (?, ?, ?);");
  stmt.bind(1, currentRoom);
  stmt.bind(2, temp);
  stmt.bind(3, hall);
  stmt.exec();
  Serial.printf("Inserted sample for room %d: temp=%.2f, hall=%.2f\n", currentRoom, temp, hall);
}
