const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <head><title>Smart Huis</title></head>
  <body>
    <h1>Beheer</h1>

    <div class="table-section">
      <h2>Gebruiker</h2>
      <form action="/addUser" method="post">
        Naam toevoegen: <input type="text" name="username">
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

    <div class="table-section">
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

    <div class="table-section">
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

    <div class="table-section">
  <h2>Koppelingen</h2>

  <form action="/linkUserRoom" method="post">
    Koppel Gebruiker ID 
    <select name="user_id">%USER_OPTIONS%</select>
    aan Kamer ID 
    <select name="room_id">%ROOM_OPTIONS%</select>
    <input type="submit" value="Koppel">
  </form>

  <form action="/linkDeviceRoom" method="post">
    Koppel Apparaat ID 
    <select name="device_id">%DEVICE_OPTIONS%</select>
    aan Kamer ID 
    <select name="room_id">%ROOM_OPTIONS%</select>
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

  </body>
</html>
)rawliteral";
