#ifndef HTML_H
#define HTML_H

const String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Room Manager</title>
  <style>
    body { font-family: Arial, sans-serif; padding: 20px; }
    input, select, button { margin: 5px 0; padding: 5px; width: 200px; }
    label { display: block; margin-top: 10px; }
    .form-actions button { margin-right: 10px; }
    #refreshSamplesBtn { margin-top: 10px; padding: 5px 10px; width: auto; }
  </style>
  <script>
    let rooms = [];

    function fetchRooms() {
      fetch('/getRooms')
        .then(res => res.json())
        .then(data => {
          rooms = data;
          updateRoomDropdown();
        });
    }

    function updateRoomDropdown() {
      const select = document.getElementById('room_id');
      select.innerHTML = '<option value="">-- New Room --</option>';
      rooms.forEach(room => {
        const option = document.createElement('option');
        option.value = room.id;
        option.textContent = `ID ${room.id}`;
        select.appendChild(option);
      });
    }

    function onRoomSelect() {
      const id = document.getElementById('room_id').value;
      const room = rooms.find(r => r.id == id);
      document.getElementById('roomname').value = room?.name || '';
      document.getElementById('floor').value = room?.floor || '';
      document.getElementById('owner').value = room?.owner || '';
    }

    function submitForm(action) {
      const id = document.getElementById('room_id').value;
      const name = document.getElementById('roomname').value;
      const floor = document.getElementById('floor').value;
      const owner = document.getElementById('owner').value;

      let url = '';
      let body = {};

      if (action === 'add') {
        url = '/addRoom';
        body = { name, floor, owner };
      } else if (action === 'update') {
        if (!id) return alert('Select a room ID to update.');
        url = '/updateRoom';
        body = { id, name, floor, owner };
      } else if (action === 'delete') {
        if (!id) return alert('Select a room ID to delete.');
        url = '/deleteRoom';
        body = { id };
      }

      fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: Object.keys(body).map(k => k + '=' + encodeURIComponent(body[k])).join('&')
      })
      .then(() => {
        alert(action.charAt(0).toUpperCase() + action.slice(1) + ' successful!');
        fetchRooms();
        if (action !== 'update') {
          document.getElementById('room_id').value = '';
          document.getElementById('roomname').value = '';
          document.getElementById('floor').value = '';
          document.getElementById('owner').value = '';
        }
      });
    }

    function fetchSamples() {
      fetch('/refreshSamples')
        .then(res => res.text())
        .then(html => {
          document.getElementById('samplesContainer').innerHTML = html;
        });
    }

    window.onload = () => {
      fetchRooms();
      fetchSamples(); // load samples on page load
    };
  </script>
</head>
<body>
  <h1>Room Manager</h1>

  <label for="room_id">Select Room ID:</label>
  <select id="room_id" onchange="onRoomSelect()">
    <option value="">-- Loading... --</option>
  </select>

  <label for="roomname">Room Name:</label>
  <input type="text" id="roomname" required>

  <label for="floor">Floor:</label>
  <input type="number" id="floor" required>

  <label for="owner">Owner:</label>
  <input type="text" id="owner" required>

  <div class="form-actions">
    <button onclick="submitForm('add')">Add</button>
    <button onclick="submitForm('update')">Update</button>
    <button onclick="submitForm('delete')">Delete</button>
  </div>

  <h2>Recent Samples</h2>
  <button id="refreshSamplesBtn" onclick="fetchSamples()">Refresh Samples</button>
  <div id="samplesContainer">Loading samples...</div>

</body>
</html>
)rawliteral";

#endif // HTML_H
