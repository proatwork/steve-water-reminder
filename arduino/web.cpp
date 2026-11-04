#include "web.h"
#include "water.h"
#include "display.h"
#include <Arduino.h>
#include <stdarg.h>

static const int LOG_SIZE = 60;
static String logBuffer[LOG_SIZE];
static int logHead = 0;
static int logCount = 0;

static const char* CONFIG_FILE = "/config.txt";

void saveConfig() {
  if (SD.exists(CONFIG_FILE)) SD.remove(CONFIG_FILE);
  File f = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!f) { Serial.println("[config] failed to save"); return; }
  f.printf("trigger_cm:%d\n", triggerCm);
  f.printf("motion_cooldown_ms:%lu\n", motionCooldownMs);
  f.printf("water_cooldown_ms:%lu\n", waterCooldownMs);
  f.printf("active_start:%d\n", activeStartHour);
  f.printf("active_end:%d\n", activeEndHour);
  f.close();
  Serial.println("[config] saved");
}

void loadConfig() {
  if (!SD.exists(CONFIG_FILE)) return;
  File f = SD.open(CONFIG_FILE, FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.startsWith("trigger_cm:"))        triggerCm = line.substring(11).toInt();
    else if (line.startsWith("motion_cooldown_ms:")) motionCooldownMs = strtoul(line.substring(19).c_str(), nullptr, 10);
    else if (line.startsWith("water_cooldown_ms:"))  waterCooldownMs = strtoul(line.substring(18).c_str(), nullptr, 10);
    else if (line.startsWith("active_start:"))  activeStartHour = line.substring(13).toInt();
    else if (line.startsWith("active_end:"))    activeEndHour = line.substring(11).toInt();
  }
  f.close();
  Serial.printf("[config] loaded dist=%d cooldown=%lums water=%lums hours=%d-%d\n",
    triggerCm, motionCooldownMs, waterCooldownMs, activeStartHour, activeEndHour);
}

void webLog(const char* fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.print(buf);
  String line = buf;
  line.trim();
  if (line.length() == 0) return;
  logBuffer[logHead] = line;
  logHead = (logHead + 1) % LOG_SIZE;
  if (logCount < LOG_SIZE) logCount++;
}

void handleRoot() {
  String html = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<style>"
      "body{font-family:sans-serif;padding:1em;max-width:800px;margin:auto;background:#111;color:#eee}"
      "h2{text-align:center;margin:0.5em 0}"
      "h3{margin:1.2em 0 0.4em;border-bottom:1px solid #333;padding-bottom:0.3em}"
      ".controls{text-align:center;margin-bottom:1em}"
      ".btn{font-size:1.5em;padding:0.6em 1.5em;margin:0.3em;border:none;border-radius:10px;color:#fff;cursor:pointer}"
      ".btn-det{background:#4CAF50}.btn-sch{background:#2196F3}.btn-stop{background:#f44336}.btn-status{background:#555}"
      ".cfg{display:flex;gap:1em;flex-wrap:wrap;margin:0.5em 0}"
      ".cfg label{display:flex;align-items:center;gap:0.4em}"
      ".cfg input[type=number]{width:5em;padding:0.3em;border-radius:6px;border:1px solid #555;background:#222;color:#eee}"
      ".cfg button{padding:0.3em 1em;border:none;border-radius:6px;background:#ff9800;color:#fff;cursor:pointer}"
      "#st{text-align:left;background:#222;color:#0f0;padding:0.8em;border-radius:8px;overflow:auto;font-size:0.85em;margin:0.5em 0}"
      ".fm-bar{display:flex;gap:0.5em;align-items:center;flex-wrap:wrap;margin:0.5em 0}"
      ".fm-bar select,.fm-bar button{padding:0.3em 0.8em;border-radius:6px;border:1px solid #555;background:#222;color:#eee;cursor:pointer}"
      "#upform{margin:0.5em 0}"
      "#upform input[type=file]{color:#eee}"
      "#upform button{padding:0.3em 1em;border:none;border-radius:6px;background:#2196F3;color:#fff;cursor:pointer}"
      "#files{list-style:none;padding:0;margin:0.5em 0}"
      "#files li{padding:0.4em;border-bottom:1px solid #333;display:flex;align-items:center;justify-content:space-between}"
      "#files button{padding:0.2em 0.6em;border:none;border-radius:4px;cursor:pointer;margin-left:0.3em}"
      ".play-btn{background:#4CAF50;color:#fff}.del-btn{background:#c62828;color:#fff}"
      ".water{display:flex;gap:1.5em;align-items:center;flex-wrap:wrap;margin:0.5em 0}"
      ".water-kid{background:#222;border-radius:10px;padding:0.8em 1.2em;text-align:center;min-width:100px}"
      ".water-kid .name{font-weight:bold;font-size:1.1em;margin-bottom:0.3em}"
      ".water-kid .count{font-size:2em;color:#4fc3f7}"
      ".water-ctrl{display:flex;align-items:center;gap:0.4em;justify-content:center}"
      ".water-btn{width:2em;height:2em;font-size:1.2em;font-weight:bold;border:none;border-radius:6px;background:#555;color:#fff;cursor:pointer}"
      ".water-reset{padding:0.3em 1em;border:none;border-radius:6px;background:#c62828;color:#fff;cursor:pointer}"
    "</style>"
    "<body>"
    "<h2>Steve</h2>";

  html += "<div class='controls'>"
    "<button class='btn btn-det' onclick=\"fetch('/detection')\">DETECTION</button>"
    "<button class='btn btn-sch' onclick=\"fetch('/scheduled')\">SCHEDULED</button><br>"
    "<button class='btn btn-stop' onclick=\"fetch('/stop')\">STOP</button>"
    "<button class='btn btn-status' onclick='loadStatus()'>STATUS</button>"
    "</div>"
    "<pre id='st'>Distance: " + String(triggerCm) + " cm | Cooldown: " + String((unsigned long)(motionCooldownMs / 1000)) + " s | "
      + String(isActiveHours() ? "ACTIVE" : "INACTIVE") + "</pre>";

  html += "<h3>Configuration</h3>"
    "<div class='cfg'>"
      "<label>Distance (cm): <input type='number' id='cfgDist' value='" + String(triggerCm) + "' min='1' max='400'>"
      "<button onclick=\"setCfg('/set_distance?cm='+document.getElementById('cfgDist').value)\">Set</button></label>"
      "<label>Cooldown (s): <input type='number' id='cfgCool' value='" + String((unsigned long)(motionCooldownMs / 1000)) + "' min='0' max='3600'>"
      "<button onclick=\"setCfg('/set_cooldown?seconds='+document.getElementById('cfgCool').value)\">Set</button></label>"
      "<label>Water cooldown (min): <input type='number' id='cfgWCool' value='" + String((unsigned long)(waterCooldownMs / 60000)) + "' min='0' max='120'>"
      "<button onclick=\"setCfg('/set_water_cooldown?minutes='+document.getElementById('cfgWCool').value)\">Set</button></label>"
      "<label>Active hours: <input type='number' id='cfgStart' value='" + String(activeStartHour) + "' min='0' max='23' style='width:3em'>"
      " to <input type='number' id='cfgEnd' value='" + String(activeEndHour) + "' min='0' max='23' style='width:3em'>"
      "<button onclick=\"setCfg('/set_active_hours?start='+document.getElementById('cfgStart').value+'&end='+document.getElementById('cfgEnd').value)\">Set</button></label>"
      "<label>Backlight: <input type='range' id='cfgBL' min='0' max='100' value='100' "
        "oninput=\"document.getElementById('blVal').textContent=this.value;fetch('/set_backlight?v='+this.value)\" "
        "style='width:8em;vertical-align:middle'>"
      "<span id='blVal'>100</span>%</label>"
    "</div>";

  html += "<h3>Water Tracker</h3>"
    "<div class='water'>"
      "<div class='water-kid'>"
        "<div class='name'>Timmy</div>"
        "<div class='water-ctrl'>"
          "<button class='water-btn' onclick=\"adjWater('timmy',-1)\">-</button>"
          "<span class='count' id='wTimmy'>" + String(waterTimmy) + "</span>"
          "<button class='water-btn' onclick=\"adjWater('timmy',1)\">+</button>"
        "</div>"
      "</div>"
      "<div class='water-kid'>"
        "<div class='name'>Luke</div>"
        "<div class='water-ctrl'>"
          "<button class='water-btn' onclick=\"adjWater('luke',-1)\">-</button>"
          "<span class='count' id='wLuke'>" + String(waterLuke) + "</span>"
          "<button class='water-btn' onclick=\"adjWater('luke',1)\">+</button>"
        "</div>"
      "</div>"
      "<button class='water-reset' onclick=\"if(confirm('Reset both counters?'))fetch('/water_reset',{method:'POST'}).then(()=>refreshWater())\">Reset</button>"
    "</div>";

  html += "<h3>File Manager</h3>"
    "<div class='fm-bar'>"
      "<label>Folder: <select id='folder'></select></label>"
      "<button onclick='refresh()'>Refresh</button>"
      "<button id='delFolderBtn' style='display:none;background:#c62828;color:#fff' onclick='delFolder()'>Delete folder</button>"
      "<input type='text' id='newFolder' placeholder='new folder name' style='padding:0.3em;border-radius:6px;border:1px solid #555;background:#222;color:#eee;width:10em'>"
      "<button onclick='mkDir()' style='padding:0.3em 0.8em;border-radius:6px;border:1px solid #555;background:#4CAF50;color:#fff;cursor:pointer'>Create</button>"
    "</div>"
    "<form id='upform' enctype='multipart/form-data' method='POST'>"
      "<input type='file' name='file' accept='.wav' multiple required> "
      "<button type='submit'>Upload .wav(s)</button>"
    "</form>"
    "<ul id='files'></ul>";

  html += "<script>"
    "function adjWater(name,delta){"
      "fetch('/water_set?name='+name+'&delta='+delta,{method:'POST'}).then(()=>refreshWater());"
    "}"
    "function refreshWater(){"
      "fetch('/water').then(r=>r.json()).then(d=>{"
        "document.getElementById('wTimmy').textContent=d.timmy;"
        "document.getElementById('wLuke').textContent=d.luke;"
      "});"
    "}"
    "setInterval(refreshWater,2000);"
    "function loadStatus(){"
      "fetch('/status').then(r=>r.json()).then(d=>{"
        "document.getElementById('st').textContent="
          "'Distance: '+d.trigger_cm+' cm | Cooldown: '+d.motion_cooldown_s+' s | Playing: '+d.playing+' | '+(d.active?'ACTIVE':'INACTIVE');"
        "document.getElementById('cfgDist').value=d.trigger_cm;"
        "document.getElementById('cfgCool').value=d.motion_cooldown_s;"
      "});"
    "}"
    "function setCfg(url){fetch(url).then(()=>loadStatus());}"
    "const sel=document.getElementById('folder');"
    "const form=document.getElementById('upform');"
    "function loadFolders(){"
      "fetch('/folders').then(r=>r.json()).then(d=>{"
        "const cur=sel.value;"
        "sel.innerHTML='<option value=\"\">/ (root)</option>';"
        "d.folders.forEach(f=>{"
          "const o=document.createElement('option');o.value='/'+f;o.textContent='/'+f;"
          "sel.appendChild(o);"
        "});"
        "sel.value=cur;syncUploadUrl();"
      "});"
    "}"
    "function mkDir(){"
      "const n=document.getElementById('newFolder').value.trim();"
      "if(!n)return;"
      "fetch('/mkdir?name='+encodeURIComponent(n),{method:'POST'}).then(r=>{"
        "if(r.ok){document.getElementById('newFolder').value='';loadFolders();}"
      "});"
    "}"
    "function syncUploadUrl(){form.action='/upload?folder='+encodeURIComponent(sel.value||'/');}"
    "sel.onchange=()=>{syncUploadUrl();refresh();};"
    "loadFolders();"
    "function refresh(){"
      "fetch('/list?folder='+encodeURIComponent(sel.value||'/')).then(r=>r.json()).then(d=>{"
        "const ul=document.getElementById('files');ul.innerHTML='';"
        "if(!d.files||!d.files.length){ul.innerHTML='<li><em>empty</em></li>';return;}"
        "d.files.forEach(f=>{"
          "const path=(sel.value||'')+'/'+f.name;"
          "const li=document.createElement('li');"
          "li.innerHTML=`<span><strong>${f.name}</strong> <span style='color:#888'>(${f.size} B)</span></span>"
            "<span><button class='play-btn' onclick=\"play('${path}')\">play</button>"
            "<button class='del-btn' onclick=\"del('${path}')\">delete</button></span>`;"
          "ul.appendChild(li);"
        "});"
      "});"
    "}"
    "function play(p){fetch('/playpath?path='+encodeURIComponent(p));}"
    "function del(p){if(!confirm('Delete '+p+'?'))return;fetch('/delete?path='+encodeURIComponent(p),{method:'POST'}).then(()=>refresh());}"
    "function delFolder(){"
      "const f=sel.value;if(!f)return;"
      "if(!confirm('Delete ALL files in '+f+'?'))return;"
      "fetch('/deletefolder?folder='+encodeURIComponent(f),{method:'POST'}).then(()=>{loadFolders();refresh();});"
    "}"
    "function updateDelBtn(){document.getElementById('delFolderBtn').style.display=sel.value?'inline-block':'none';}"
    "sel.addEventListener('change',updateDelBtn);updateDelBtn();"
    "refresh();"
    "</script>"
    "<h3>Serial Log</h3>"
    "<iframe src='/serial' style='width:100%;height:200px;border:1px solid #333;border-radius:8px;background:#111'></iframe>"
    "</body>";

  server.send(200, "text/html", html);
}

void handleList() {
  String folder = server.arg("folder");
  if (folder.length() == 0) folder = "/";

  File dir = SD.open(folder);
  if (!dir || !dir.isDirectory()) {
    server.send(404, "application/json", "{\"error\":\"not a directory\"}");
    return;
  }

  String json = "{\"folder\":\"" + folder + "\",\"files\":[";
  bool first = true;
  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (!first) json += ",";
      json += "{\"name\":\"" + name + "\",\"size\":" + String(entry.size()) + "}";
      first = false;
    }
    entry.close();
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleUploadChunk() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    String folder = server.arg("folder");
    if (folder.length() == 0) folder = "/";
    if (folder != "/" && !SD.exists(folder)) SD.mkdir(folder);
    if (!folder.endsWith("/")) folder += "/";
    uploadPath = folder + up.filename;
    Serial.printf("[upload] start: %s\n", uploadPath.c_str());
    if (SD.exists(uploadPath)) SD.remove(uploadPath);
    uploadFile = SD.open(uploadPath, FILE_WRITE);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
    Serial.printf("[upload] done: %u bytes -> %s\n", up.totalSize, uploadPath.c_str());
  }
}

void handleUploadDone() {
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelete() {
  String path = server.arg("path");
  if (path.length() == 0) { server.send(400, "text/plain", "missing ?path="); return; }
  if (SD.exists(path)) { SD.remove(path); server.send(200, "text/plain", "deleted"); }
  else                 { server.send(404, "text/plain", "not found"); }
}

void handleDeleteFolder() {
  String folder = server.arg("folder");
  if (folder.length() == 0 || folder == "/") {
    server.send(400, "text/plain", "cannot delete root");
    return;
  }
  File dir = SD.open(folder);
  if (!dir || !dir.isDirectory()) {
    server.send(404, "text/plain", "not a directory");
    return;
  }
  String prefix = folder.endsWith("/") ? folder : folder + "/";
  static const int MAX_ENTRIES = 64;
  String paths[MAX_ENTRIES];
  int count = 0;
  File entry;
  while ((entry = dir.openNextFile()) && count < MAX_ENTRIES) {
    String name = entry.name();
    if (name.indexOf('/') < 0) name = prefix + name;
    paths[count++] = name;
    entry.close();
  }
  dir.close();
  for (int i = 0; i < count; i++) {
    SD.remove(paths[i]);
  }
  SD.rmdir(folder);
  Serial.printf("[delete] removed %d files from %s\n", count, folder.c_str());
  server.send(200, "text/plain", String("deleted ") + count + " files");
}

void handlePlayPath() {
  if (wav && wav->isRunning()) { server.send(200, "text/plain", "busy"); return; }
  String path = server.arg("path");
  if (path.length() == 0) { server.send(400, "text/plain", "missing ?path="); return; }
  char buf[128];
  path.toCharArray(buf, sizeof(buf));
  playFile(buf);
  server.send(200, "text/plain", "playing");
}

void handleDetection() {
  if (wav && wav->isRunning()) { server.send(200, "text/plain", "busy"); return; }
  playRandom("detection");
  server.send(200, "text/plain", "detection");
}

void handleScheduled() {
  if (wav && wav->isRunning()) { server.send(200, "text/plain", "busy"); return; }
  playRandom("scheduled");
  server.send(200, "text/plain", "scheduled");
}

void handleStop() {
  cleanup();
  server.send(200, "text/plain", "stopped");
}

void handleSetCooldown() {
  if (!server.hasArg("seconds")) {
    server.send(400, "text/plain", "missing ?seconds=");
    return;
  }
  unsigned long sec = strtoul(server.arg("seconds").c_str(), nullptr, 10);
  motionCooldownMs = sec * 1000UL;
  saveConfig();
  Serial.printf("[config] motion cooldown = %lu s\n", sec);
  server.send(200, "text/plain", String("cooldown=") + sec + "s");
}

void handleSetDistance() {
  if (!server.hasArg("cm")) {
    server.send(400, "text/plain", "missing ?cm=");
    return;
  }
  triggerCm = atoi(server.arg("cm").c_str());
  saveConfig();
  Serial.printf("[config] trigger distance = %d cm\n", triggerCm);
  server.send(200, "text/plain", String("distance=") + triggerCm + "cm");
}

void handleStatus() {
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"trigger_cm\":%d,\"motion_cooldown_s\":%lu,\"playing\":%s,\"active\":%s}",
    triggerCm, motionCooldownMs / 1000,
    (wav && wav->isRunning()) ? "true" : "false",
    isActiveHours() ? "true" : "false");
  server.send(200, "application/json", buf);
}

void handleWater() {
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"timmy\":%d,\"luke\":%d}", waterTimmy, waterLuke);
  server.send(200, "application/json", buf);
}

void handleWaterSet() {
  String name = server.arg("name");
  int delta = server.arg("delta").toInt();
  int *counter;
  unsigned long *lastInc;
  if (name == "timmy") {
    counter = &waterTimmy;
    lastInc = &lastIncrementTimmy;
  } else if (name == "luke") {
    counter = &waterLuke;
    lastInc = &lastIncrementLuke;
  } else {
    server.send(400, "text/plain", "invalid name");
    return;
  }
  if (delta > 0) {
    *lastInc = millis();
    String folder = "congratulations_" + name;
    congratsPlaying = playRandom(folder.c_str());
  }
  *counter = max(0, *counter + delta);
  saveWaterCounts();
  server.send(200, "text/plain", "ok");
}

void handleWaterReset() {
  waterTimmy = 0;
  waterLuke = 0;
  saveWaterCounts();
  Serial.println("[water] counters reset");
  server.send(200, "text/plain", "reset");
}

void handleFolders() {
  File root = SD.open("/");
  if (!root) { server.send(500, "application/json", "{\"folders\":[]}"); return; }
  String json = "{\"folders\":[";
  bool first = true;
  File entry;
  while ((entry = root.openNextFile())) {
    if (entry.isDirectory()) {
      String name = entry.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (!first) json += ",";
      json += "\"" + name + "\"";
      first = false;
    }
    entry.close();
  }
  root.close();
  json += "]}";
  server.send(200, "application/json", json);
}

void handleMkdir() {
  String name = server.arg("name");
  name.trim();
  if (name.length() == 0) { server.send(400, "text/plain", "missing name"); return; }
  String path = "/" + name;
  if (SD.exists(path)) { server.send(409, "text/plain", "already exists"); return; }
  SD.mkdir(path);
  Serial.printf("[fs] created folder %s\n", path.c_str());
  server.send(200, "text/plain", "created");
}

void handleSetActiveHours() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "text/plain", "missing ?start=&end=");
    return;
  }
  activeStartHour = constrain(server.arg("start").toInt(), 0, 23);
  activeEndHour = constrain(server.arg("end").toInt(), 0, 23);
  saveConfig();
  webLog("[config] active hours = %d:00 - %d:00\n", activeStartHour, activeEndHour);
  server.send(200, "text/plain", String("active=") + activeStartHour + "-" + activeEndHour);
}

void handleSetWaterCooldown() {
  if (!server.hasArg("minutes")) { server.send(400, "text/plain", "missing ?minutes="); return; }
  unsigned long mins = strtoul(server.arg("minutes").c_str(), nullptr, 10);
  waterCooldownMs = mins * 60000UL;
  saveConfig();
  Serial.printf("[config] water cooldown = %lu min\n", mins);
  server.send(200, "text/plain", String("water_cooldown=") + mins + "min");
}

void handleSerial() {
  String html = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<style>body{background:#111;color:#0f0;font-family:monospace;font-size:0.85em;padding:0.5em;margin:0}"
    "pre{white-space:pre-wrap;word-break:break-all}</style>"
    "<body><pre id='log'></pre>"
    "<script>"
    "function poll(){fetch('/serial_data').then(r=>r.text()).then(t=>{document.getElementById('log').textContent=t;window.scrollTo(0,document.body.scrollHeight);});}"
    "setInterval(poll,1500);poll();"
    "</script></body>";
  server.send(200, "text/html", html);
}

void handleSerialData() {
  String out;
  int start = (logCount < LOG_SIZE) ? 0 : logHead;
  for (int j = 0; j < logCount; j++) {
    int idx = (start + j) % LOG_SIZE;
    out += logBuffer[idx] + "\n";
  }
  server.send(200, "text/plain", out);
}

void handleSetBacklight() {
  if (!server.hasArg("v")) { server.send(400, "text/plain", "missing ?v="); return; }
  int v = server.arg("v").toInt();
  setBacklight(v);
  server.send(200, "text/plain", "ok");
}
