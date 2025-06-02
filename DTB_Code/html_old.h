const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Smart Huis</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 20px;
        background-color: #f9f9f9;
      }

      h1, h2 {
        color: #333;
      }

      .tabs button {
        padding: 10px 20px;
        margin-right: 5px;
        border: none;
        background-color: #ddd;
        cursor: pointer;
        font-weight: bold;
      }

      .tabs button.active {
        background-color: #bbb;
      }

      .tab-content {
        display: none;
        margin-top: 20px;
        padding: 15px;
        border: 1px solid #ccc;
        background-color: white;
        border-radius: 5px;
      }

      form {
        margin-bottom: 15px;
      }

      input[type="text"], select {
        padding: 5px;
        margin-left: 10px;
        min-width: 200px;
      }

      input[type="submit"] {
        padding: 5px 10px;
        margin-left: 10px;
        background-color: #4CAF50;
        color: white;
        border: none;
        cursor: pointer;
      }

      input[type="submit"]:hover {
        background-color: #45a049;
      }

      table {
        margin-top: 20px;
      }

      td {
        padding: 5px 10px;
      }
    </style>
    <script>
      function showTab(tabId) {
        var tabs = document.getElementsByClassName("tab-content");
        for (var i = 0; i < tabs.length; i++) {
          tabs[i].style.display = "none";
        }
        document.getElementById(tabId).style.display = "block";

        var buttons = document.querySelectorAll(".tabs button");
        buttons.forEach(btn => btn.classList.remove("active"));
        document.getElementById("btn-" + tabId).classList.add("active");
      }

      window.onload = function() {
        showTab('user-tab');
      };
    </script>
  </head>
  <body>
    <h1>Smart Huis Beheer</h1>

    <div class="tabs">
      <button id="btn-user-tab" onclick="showTab('user-tab')">Gebruiker</button>
      <button id="btn-room-tab" onclick="showTab('room-tab')">Kamer</button>
      <button id="btn-device-tab" onclick="showTab('device-tab')">Apparaat</button>
      <button id="btn-link-tab" onclick="showTab('link-tab')">Koppelingen</button>
      <button id="btn-view-tab" onclick="showTab('view-tab')">Download/Bekijk</button>
    </div>

    <div id="user-tab" class="tab-content">
      <h2>Gebruiker</h2>
      <form action="/addUser" method="post">
        Naam toevoegen: <input type="text" name="username">
        Geboortedatum: <input type="date" name="b_date">
        <input type="submit" value="Voeg toe">
      </form>
      <form action="/deleteUser" method="post">
        Naam verwijderen (ID): <select name="user_id">%USER_OPTIONS%</select>
        <input type="submit" value="Verwijder">
      </form>
      <form action="/clearUsers" method="post">
        <input type="submit" value="Maak gebruiker tabel leeg">
      </form>
    </div>

    <div id="room-tab" class="tab-content">
      <h2>Kamer</h2>
      <form action="/addRoom" method="post">
        Naam toevoegen: <input type="text" name="roomname">
        <input type="submit" value="Voeg toe">
      </form>
      <form action="/deleteRoom" method="post">
        Naam verwijderen (ID): <select name="room_id">%ROOM_OPTIONS%</select>
        <input type="submit" value="Verwijder">
      </form>
      <form action="/clearRooms" method="post">
        <input type="submit" value="Maak kamer tabel leeg">
      </form>
    </div>

    <div id="device-tab" class="tab-content">
      <h2>Apparaat</h2>
      <form action="/addDevice" method="post">
        Naam toevoegen: <input type="text" name="devicename">
        <input type="submit" value="Voeg toe">
      </form>
      <form action="/deleteDevice" method="post">
        Naam verwijderen (ID): <select name="device_id">%DEVICE_OPTIONS%</select>
        <input type="submit" value="Verwijder">
      </form>
      <form action="/clearDevices" method="post">
        <input type="submit" value="Maak apparaat tabel leeg">
      </form>
    </div>

    <div id="link-tab" class="tab-content">
      <h2>Koppelingen</h2>

      <form action="/linkUserRoom" method="post">
        Koppel Gebruiker ID <select name="user_id">%USER_OPTIONS%</select>
        aan Kamer ID <select name="room_id">%ROOM_OPTIONS%</select>
        <input type="submit" value="Koppel">
      </form>

      <form action="/linkDeviceRoom" method="post">
        Koppel Apparaat ID <select name="device_id">%DEVICE_OPTIONS%</select>
        aan Kamer ID <select name="room_id">%ROOM_OPTIONS%</select>
        <input type="submit" value="Koppel">
      </form>

      <form action="/deleteUserRoom" method="post">
        Verwijder Gebruiker - Kamer koppeling  
        <select name="user_room_coupling">%USER_ROOM_OPTIONS%</select>
        <input type="submit" value="Verwijder koppeling">
      </form>

      <form action="/deleteDeviceRoom" method="post">
        Verwijder Apparaat - Kamer koppeling  
        <select name="device_room_coupling">%DEVICE_ROOM_OPTIONS%</select>
        <input type="submit" value="Verwijder koppeling">
      </form>

      <form action="/clearUserRoom" method="post">
        <input type="submit" value="Maak alle gebruiker-kamer koppelingen leeg">
      </form>

      <form action="/clearDeviceRoom" method="post">
        <input type="submit" value="Maak alle apparaat-kamer koppelingen leeg">
      </form>
    </div>

    <div id="view-tab" class="tab-content">
      <h2>Bekijk of download tabellen</h2>
      <table>
        <tr>
          <td>
            <form action="/downloadUsers" method="get">
              <input type="submit" value="Download Gebruikers CSV">
            </form>
          </td>
          <td>
            <form action="/viewUsers" method="get" target="_blank">
              <input type="submit" value="Bekijk">
            </form>
          </td>
        </tr>
        <tr>
          <td>
            <form action="/downloadRooms" method="get">
              <input type="submit" value="Download Kamers CSV">
            </form>
          </td>
          <td>
            <form action="/viewRooms" method="get" target="_blank">
              <input type="submit" value="Bekijk">
            </form>
          </td>
        </tr>
        <tr>
          <td>
            <form action="/downloadDevices" method="get">
              <input type="submit" value="Download Apparaten CSV">
            </form>
          </td>
          <td>
            <form action="/viewDevices" method="get" target="_blank">
              <input type="submit" value="Bekijk">
            </form>
          </td>
        </tr>
        <tr>
          <td>
            <form action="/downloadUserRoom" method="get">
              <input type="submit" value="Download Gebruiker-Kamer CSV">
            </form>
          </td>
          <td>
            <form action="/viewUserRoom" method="get" target="_blank">
              <input type="submit" value="Bekijk">
            </form>
          </td>
        </tr>
        <tr>
          <td>
            <form action="/downloadDeviceRoom" method="get">
              <input type="submit" value="Download Apparaat-Kamer CSV">
            </form>
          </td>
          <td>
            <form action="/viewDeviceRoom" method="get" target="_blank">
              <input type="submit" value="Bekijk">
            </form>
          </td>
        </tr>
      </table>
    </div>
  </body>
</html>
)rawliteral";
