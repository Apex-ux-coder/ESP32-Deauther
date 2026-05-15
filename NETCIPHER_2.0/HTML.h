#ifndef HTML_H
#define HTML_H

#include <Arduino.h>
#include <WebServer.h>
#include <U8g2lib.h>

extern WebServer webServer;
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>NETCIPHER</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#111;color:#ccc;font-family:'Courier New',monospace;font-size:13px;min-height:100vh}
#hdr{background:#1a1a1a;border-bottom:1px solid #333;padding:6px 10px;display:flex;justify-content:space-between;align-items:center}
#hdr .logo{color:#fff;font-size:15px;font-weight:bold;letter-spacing:2px}
#hdr .meta{text-align:right;color:#666;font-size:11px;line-height:1.5}
#hdr .meta span{color:#5af}
#devbar{background:#0d0d0d;border-bottom:1px solid #1e1e1e;padding:4px 10px;display:flex;gap:20px;font-size:11px;color:#444}
#devbar span{color:#666}
#tabs{display:flex;border-bottom:1px solid #333;background:#111}
.tab{padding:8px 16px;cursor:pointer;color:#666;font-size:12px;letter-spacing:1px;border-bottom:2px solid transparent;transition:all .15s}
.tab:hover{color:#ccc;background:#1a1a1a}
.tab.active{color:#5af;border-bottom-color:#5af;background:#0d1a26}
.page{display:none;padding:10px}
.page.active{display:block}
.sec{color:#5af;font-size:11px;letter-spacing:1px;margin-bottom:6px;padding-bottom:4px;border-bottom:1px solid #222}
.tbl-wrap{overflow-x:auto;margin-bottom:12px;border:1px solid #2a2a2a;border-radius:3px}
table{width:100%;border-collapse:collapse;font-size:12px}
thead th{background:#1e1e1e;color:#888;padding:6px 8px;text-align:left;border-bottom:1px solid #2a2a2a;font-weight:normal;letter-spacing:.5px;white-space:nowrap}
tbody td{padding:6px 8px;border-bottom:1px solid #1e1e1e;vertical-align:middle}
tbody tr{cursor:pointer;transition:background .1s}
tbody tr:hover{background:#1a2030}
tbody tr.sel{background:#0d2040;color:#fff}
tbody tr.sel td{color:#7df}
.empty{text-align:center;color:#444;padding:16px!important}
.row-btns{display:flex;gap:6px;margin-bottom:8px;align-items:center}
.btn{background:#1a1a1a;border:1px solid #333;color:#bbb;padding:5px 10px;font-family:'Courier New',monospace;font-size:11px;cursor:pointer;letter-spacing:.5px;border-radius:2px;transition:all .15s}
.btn:hover{background:#252525;border-color:#555;color:#fff}
.btn:active{background:#1a1a1a;border-color:#777;transform:scale(0.98);box-shadow:inset 0 1px 2px rgba(0,0,0,0.5)}
.btn.scan-active{border-color:#fa0;color:#fa0}
.btn.sm{padding:3px 8px;font-size:10px}
.btn.danger{border-color:#a33;color:#c66}
.btn.danger:hover{background:#2a1010;border-color:#c55}
.atk-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:6px;margin-bottom:12px}
.atk-btn{background:#1a1a1a;border:1px solid #2a2a2a;color:#aaa;padding:10px 8px;font-family:'Courier New',monospace;font-size:11px;cursor:pointer;letter-spacing:.5px;border-radius:2px;text-align:center;transition:all .15s;line-height:1.3}
.atk-btn:hover{background:#222;border-color:#444;color:#ddd}
.atk-btn:active{transform:scale(0.96);box-shadow:inset 0 2px 3px rgba(0,0,0,0.6)}
.atk-btn.on{background:#0a1826;border-color:#5af;color:#5af}
.atk-btn.red{border-color:#2a1010}
.atk-btn.red:hover{border-color:#a33}
.atk-btn.red.on{background:#1e0a0a;border-color:#c33;color:#f66}
.pill{display:inline-block;font-size:10px;padding:1px 6px;border-radius:2px;letter-spacing:.5px}
.pill.off{background:#1a1a1a;color:#555;border:1px solid #2a2a2a}
.pill.on{background:#0a1826;color:#5af;border:1px solid #25a}
.pill.run{background:#1a1a0a;color:#ca0;border:1px solid #430}
.s5{color:#4f4}.s4{color:#8f4}.s3{color:#ff4}.s2{color:#f84}.s1{color:#f44}
.log-ssid{color:#5af;min-width:120px}
.log-pw{color:#fff;font-family:'Courier New',monospace}
.log-ok{color:#4c4;font-size:10px}
.log-no{color:#a44;font-size:10px}

::-webkit-scrollbar{width:4px;height:4px}
::-webkit-scrollbar-track{background:#111}
::-webkit-scrollbar-thumb{background:#333}
</style>
</head>
<body>

<div id="hdr">
  <div class="logo">NETCIPHER</div>
  <div class="meta">
    <div id="tdisp">--:--:--</div>
    <div id="ddisp">--- --, ----</div>
  </div>
</div>

<div id="devbar">
  <span>AP: <span id="apMac">--:--:--:--:--:--</span></span>
  <span>STA: <span id="staMac">--:--:--:--:--:--</span></span>
  <span id="statusPill" style="margin-left:auto"><span class="pill off">IDLE</span></span>
</div>

<div id="tabs">
  <div class="tab active" onclick="switchTab('scan')">SCAN</div>
  <div class="tab" onclick="switchTab('attack')">ATTACK</div>
  <div class="tab" onclick="switchTab('logs')">LOGS</div>
  <div class="tab" onclick="switchTab('settings')">SETTINGS</div>
</div>

<div id="page-scan" class="page active">
  <div class="sec">SYSTEM CONTROLS</div>
  <div class="row-btns" style="margin-bottom:12px">
    <button class="atk-btn" id="btn-DEAUTH_MON" onclick="sendCmd('DEAUTH_MON')">DEAUTH MON</button>
    <button class="atk-btn" onclick="window.open('/display_mirror','_blank')">DISPLAY MIRROR</button>
    <button class="atk-btn red" onclick="sendCmd('REBOOT')">REBOOT</button>
    <button class="atk-btn red" onclick="sendCmd('SLEEP')">SLEEP</button>
  </div>
  <div class="sec">NETWORKS <span id="apCnt" style="color:#555">(0)</span></div>
  <div class="row-btns">
    <button class="btn" id="scanBtn" onclick="doScan()">SCAN</button>
    <button class="btn sm" onclick="sendCmd('SELECT_ALL_APS')">SELECT ALL</button>
    <button class="btn sm" onclick="sendCmd('CLEAR_ALL_APS')">CLEAR</button>
  </div>
  <div class="tbl-wrap">
    <table>
      <thead><tr><th style="width:20px"></th><th>SSID</th><th>CH</th><th>SIGNAL</th></tr></thead>
      <tbody id="apBody"><tr><td class="empty" colspan="4">No networks. Click SCAN.</td></tr></tbody>
    </table>
  </div>
  <div class="sec" style="margin-top:4px">STATIONS <span id="staCnt" style="color:#555">(0)</span></div>
  <div class="row-btns">
    <button class="btn" id="staBtn" onclick="doScanSTA()">SCAN STA</button>
    <button class="btn sm" onclick="sendCmd('SELECT_ALL_STATIONS')">SELECT ALL</button>
    <button class="btn sm" onclick="sendCmd('CLEAR_ALL_STATIONS')">CLEAR</button>
  </div>
  <div class="tbl-wrap">
    <table>
      <thead><tr><th style="width:20px"></th><th>MAC</th><th>AP</th></tr></thead>
      <tbody id="staBody"><tr><td class="empty" colspan="3">No stations found. Run STA SCAN.</td></tr></tbody>
    </table>
  </div>
</div>

<div id="page-attack" class="page">
  <div class="sec" style="margin-bottom:8px">ATTACKS</div>
  <div class="atk-grid">
    <button class="atk-btn" id="btn-DEAUTH"       onclick="sendCmd('DEAUTH')">DEAUTH</button>
    <button class="atk-btn" id="btn-EVIL_TWIN"    onclick="sendCmd('EVIL_TWIN')">EVIL TWIN</button>
    <button class="atk-btn" id="btn-FLOOD"        onclick="sendCmd('FLOOD')">DEAUTH FLOOD</button>
    <button class="atk-btn" id="btn-DEAUTH_ALL"   onclick="sendCmd('DEAUTH_ALL')">DEAUTH ALL</button>
    <button class="atk-btn" id="btn-BEACON_SPAM"  onclick="sendCmd('BEACON_SPAM')">BEACON SPAM</button>
    <button class="atk-btn" id="btn-REACTIVE"     onclick="sendCmd('REACTIVE')">DYNAMIC DEAUTH</button>
    <button class="atk-btn" id="btn-RECOVER_SSID" onclick="sendCmd('RECOVER_SSID')">RECOVER SSID</button>
  </div>
  <div class="sec" style="margin-top:4px;margin-bottom:8px">STATUS</div>
  <div style="display:grid;grid-template-columns:1fr 1fr;gap:4px;font-size:11px">
    <div>DEAUTH: <span id="s-DEAUTH" class="pill off">OFF</span></div>
    <div>EVIL TWIN: <span id="s-EVIL_TWIN" class="pill off">OFF</span></div>
    <div>FLOOD: <span id="s-FLOOD" class="pill off">OFF</span></div>
    <div>DEAUTH MON: <span id="s-DEAUTH_MON" class="pill off">OFF</span></div>
    <div>DEAUTH ALL: <span id="s-DEAUTH_ALL" class="pill off">OFF</span></div>
    <div>BEACON SPAM: <span id="s-BEACON_SPAM" class="pill off">OFF</span></div>
    <div>DYNAMIC DEAUTH: <span id="s-REACTIVE" class="pill off">OFF</span></div>
    <div>RECOVER SSID: <span id="s-RECOVER_SSID" class="pill off">OFF</span></div>
  </div>
  <div class="sec" style="margin-top:8px;margin-bottom:8px">TARGETS</div>
  <div style="display:grid;grid-template-columns:1fr 1fr;gap:4px;font-size:11px">
    <div>NETWORKS: <span id="target-networks" style="color:#5af;font-weight:bold">0</span></div>
    <div>STATIONS: <span id="target-stations" style="color:#5af;font-weight:bold">0</span></div>
  </div>
</div>

<div id="page-logs" class="page">
  <div class="sec">PASSWORD LOGS</div>
  <div class="row-btns">
    <button class="btn danger sm" onclick="deleteAllLogs()">DELETE ALL</button>
    <button class="btn danger sm" onclick="deleteSelectedLogs()">DELETE SELECTED</button>
  </div>
  <div class="tbl-wrap">
    <table>
      <thead><tr>
        <th style="width:20px"><input type="checkbox" id="selAllChk" onclick="toggleSelAll()" style="cursor:pointer"></th>
        <th>SSID</th><th>PASSWORD</th><th>STATUS</th>
      </tr></thead>
      <tbody id="logsBody"><tr><td class="empty" colspan="4">No logs captured yet.</td></tr></tbody>
    </table>
  </div>
</div>

<div id="page-settings" class="page">
  <div class="sec">AP SETTINGS</div>
  <form id="apSettingsForm" onsubmit="saveAPSettings(event)" style="padding:10px">
    <div style="margin-bottom:12px">
      <label for="apNameInput" style="display:block;color:#888;font-size:11px;margin-bottom:4px;letter-spacing:0.5px">AP NAME (SSID):</label>
      <input type="text" id="apNameInput" name="apName" maxlength="32" required style="width:100%;padding:6px;background:#1a1a1a;border:1px solid #333;color:#ccc;font-family:'Courier New',monospace;font-size:12px;box-sizing:border-box;border-radius:2px">
    </div>
    <div style="margin-bottom:12px">
      <label for="apPasswordInput" style="display:block;color:#888;font-size:11px;margin-bottom:4px;letter-spacing:0.5px">PASSWORD:</label>
      <input type="text" id="apPasswordInput" name="apPassword" minlength="8" maxlength="32" required style="width:100%;padding:6px;background:#1a1a1a;border:1px solid #333;color:#ccc;font-family:'Courier New',monospace;font-size:12px;box-sizing:border-box;border-radius:2px">
    </div>
    <div style="display:flex;gap:8px">
      <button type="submit" class="btn" style="flex:1">SAVE & RESTART AP</button>
      <button type="button" class="btn" onclick="loadAPSettings()" style="flex:1">REFRESH</button>
    </div>
    <div id="settingsStatus" style="margin-top:8px;font-size:11px;color:#666;text-align:center;display:none"></div>
  </form>
</div>

<script>
var pollInterval=1000,lastHash='',pollTimer=null,fetchPending=false;
var attackStates={};
var REQUIRES_TARGET=new Set(['EVIL_TWIN','FLOOD','REACTIVE','RECOVER_SSID']);
var SELECTION_CMDS=new Set(['SELECT_ALL_APS','CLEAR_ALL_APS','SELECT_ALL_STATIONS','CLEAR_ALL_STATIONS']);
var TOGGLE_CMDS=new Set(['DEAUTH','EVIL_TWIN','FLOOD','DEAUTH_MON','DEAUTH_ALL','BEACON_SPAM','REACTIVE','RECOVER_SSID']);
var pendingCmds=new Set();

function switchTab(name){
  var names=['scan','attack','logs','settings'];
  document.querySelectorAll('.tab').forEach(function(t,i){t.classList.toggle('active',names[i]===name);});
  document.querySelectorAll('.page').forEach(function(p){p.classList.toggle('active',p.id==='page-'+name);});
  if(name==='settings')loadAPSettings();
}

function rssiPct(r){if(!r)return 60;r=Number(r);if(r>=-30)return 100;if(r<=-100)return 1;return Math.min(100,Math.round(8+((r+100)/70)*92+20));}
function sigClass(p){return p>=80?'s5':p>=60?'s4':p>=40?'s3':p>=20?'s2':'s1';}
function hasAPTarget(){return document.querySelectorAll('#apBody input[type="checkbox"]:checked').length>0;}
function hasSTATarget(){return document.querySelectorAll('#staBody input[type="checkbox"]:checked').length>0;}
function esc(t){var d=document.createElement('div');d.textContent=t;return d.innerHTML;}

function hashData(d){
  var sa=d.targets?d.targets.filter(function(t){return t.selected;}).length:0;
  var ss=d.stations?d.stations.filter(function(s){return s.selected;}).length:0;
  return (d.targets?d.targets.length:0)+'-'+(d.stations?d.stations.length:0)+'-'+sa+'-'+ss+'-'+(d.logs?d.logs.length:0)+'-'+(d.deauthActive||0)+(d.floodActive||0)+(d.deauthMonActive||0)+(d.deauthAllActive||0)+(d.beaconSpamActive||0)+(d.recoverSsidActive||0)+(d.evilTwinActive||0)+(d.reactiveActive||0);
}

function doScan(){
  var btn=document.getElementById('scanBtn');
  btn.textContent='SCANNING...';btn.classList.add('scan-active');btn.disabled=true;
  document.getElementById('apBody').innerHTML='<tr><td class="empty" colspan="4" style="color:#880">Scanning...</td></tr>';
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=SCAN_NETWORKS'})
    .then(function(r){if(!r.ok)throw new Error();scheduleFetch(2500);})
    .catch(function(e){console.error(e);})
    .finally(function(){btn.textContent='SCAN';btn.classList.remove('scan-active');btn.disabled=false;});
}

function doScanSTA(){
  var btn=document.getElementById('staBtn');
  btn.textContent='SCANNING...';btn.classList.add('scan-active');btn.disabled=true;
  document.getElementById('staBody').innerHTML='<tr><td class="empty" colspan="3" style="color:#880">Scanning...</td></tr>';
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=SCAN_STATIONS'})
    .then(function(r){if(!r.ok)throw new Error();scheduleFetch(3000);})
    .catch(function(e){console.error(e);})
    .finally(function(){btn.textContent='SCAN STA';btn.classList.remove('scan-active');btn.disabled=false;});
}

function toggleAP(bssid){
  var safe=bssid.replace(/:/g,'-');
  var cb=document.getElementById('ap-cb-'+safe);
  var row=document.getElementById('ap-row-'+safe);
  var nv=!cb.checked;
  cb.checked=nv;row.classList.toggle('sel',nv);
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=SELECT_AP_BSSID_'+bssid}).catch(function(e){console.error(e);});
}

function toggleSTA(mac){
  var safe=mac.replace(/:/g,'-');
  var cb=document.getElementById('sta-cb-'+safe);
  var row=document.getElementById('sta-row-'+safe);
  var nv=!cb.checked;
  cb.checked=nv;row.classList.toggle('sel',nv);
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=SELECT_STA_MAC_'+mac}).catch(function(e){console.error(e);});
}

function sendCmd(cmd){
  if(cmd==='DEAUTH'){if(!hasAPTarget()&&!hasSTATarget())return;}
  else if(REQUIRES_TARGET.has(cmd)&&!hasAPTarget())return;
  if(pendingCmds.has(cmd))return;
  pendingCmds.add(cmd);
  if(SELECTION_CMDS.has(cmd))lastHash='';
  
  // Optimistic UI update - show active state immediately
  if(TOGGLE_CMDS.has(cmd)){
    var btn=document.getElementById('btn-'+cmd);
    var pill=document.getElementById('s-'+cmd);
    if(btn&&pill){
      var isCurrentlyOn=btn.classList.contains('on');
      btn.classList.toggle('on',!isCurrentlyOn);
      pill.classList.toggle('on',!isCurrentlyOn);
      pill.textContent=!isCurrentlyOn?'ON':'OFF';
    }
  }
  
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(cmd)})
    .then(function(r){if(!r.ok)throw new Error();pollInterval=500;scheduleFetch();})
    .catch(function(e){console.error(e);})
    .finally(function(){setTimeout(function(){pendingCmds.delete(cmd);},500);});
}

function scheduleFetch(delay){if(pollTimer)clearTimeout(pollTimer);pollTimer=setTimeout(fetchStatus,delay||0);}

function fetchStatus(){
  if(fetchPending)return;
  fetchPending=true;
  fetch('/status').then(function(r){return r.json();}).then(function(data){
    var nh=hashData(data);
    var changed=nh!==lastHash;
    lastHash=nh;
    pollInterval=changed?500:3000;
    updateAtkUI(data);
    if(changed){
      if(data.targets)renderAPs(data.targets);
      if(data.stations)renderSTAs(data.stations);
      if(data.logs)renderLogs(data.logs);
    }
  }).catch(function(e){console.error(e);}).finally(function(){fetchPending=false;if(pollTimer)clearTimeout(pollTimer);pollTimer=setTimeout(fetchStatus,pollInterval);});
}

function renderAPs(data){
  var tbody=document.getElementById('apBody');
  document.getElementById('apCnt').textContent='('+data.length+')';
  if(!data.length){tbody.innerHTML='<tr><td class="empty" colspan="4">No networks. Click SCAN.</td></tr>';return;}
  var h='';
  for(var i=0;i<data.length;i++){
    var t=data[i],safe=t.bssid.replace(/:/g,'-'),p=rssiPct(t.rssi||(-100)),sc=sigClass(p),ssid=t.ssid||'<i>&lt;HIDDEN&gt;</i>',sel=t.selected;
    h+='<tr id="ap-row-'+safe+'" class="'+(sel?'sel':'')+' ap-row" onclick="toggleAP(\''+t.bssid+'\')" style="cursor:pointer"><td><input type="checkbox" id="ap-cb-'+safe+'" '+(sel?'checked':'')+' onclick="event.stopPropagation();toggleAP(\''+t.bssid+'\')" style="cursor:pointer"></td><td><div>'+ssid+'</div><div style="font-size:9px;color:#666;font-family:monospace;margin-top:2px">'+t.bssid+'</div></td><td>'+t.ch+'</td><td class="'+sc+'">'+p+'%</td></tr>';
  }
  tbody.innerHTML=h;
}

function renderSTAs(data){
  var tbody=document.getElementById('staBody');
  document.getElementById('staCnt').textContent='('+data.length+')';
  if(!data.length){tbody.innerHTML='<tr><td class="empty" colspan="3">No stations found. Run STA SCAN.</td></tr>';return;}
  var h='';
  for(var i=0;i<data.length;i++){
    var s=data[i],safe=s.mac.replace(/:/g,'-'),sel=s.selected;
    h+='<tr id="sta-row-'+safe+'" class="'+(sel?'sel':'')+'" onclick="toggleSTA(\''+s.mac+'\')"><td><input type="checkbox" id="sta-cb-'+safe+'" '+(sel?'checked':'')+' onclick="event.stopPropagation();toggleSTA(\''+s.mac+'\')"></td><td style="font-family:monospace">'+s.mac+'</td><td>'+s.ap+'</td></tr>';
  }
  tbody.innerHTML=h;
}

function renderLogs(data){
  var tbody=document.getElementById('logsBody');
  if(!data||!data.length){tbody.innerHTML='<tr><td class="empty" colspan="4">No logs captured yet.</td></tr>';return;}
  var h='';
  for(var i=0;i<data.length;i++){
    var l=data[i],ok=l.verified;
    h+='<tr><td><input type="checkbox" class="lchk" data-index="'+i+'" style="cursor:pointer"></td><td class="log-ssid">'+esc(l.ssid)+'</td><td class="log-pw">'+esc(l.password)+'</td><td class="'+(ok?'log-ok':'log-no')+'">'+(ok?'OK':'UNVERIFIED')+'</td></tr>';
  }
  tbody.innerHTML=h;
  var sc=document.getElementById('selAllChk');if(sc)sc.checked=false;
}

function updateAtkUI(data){
  var map={DEAUTH:'deauthActive',EVIL_TWIN:'evilTwinActive',FLOOD:'floodActive',DEAUTH_MON:'deauthMonActive',DEAUTH_ALL:'deauthAllActive',BEACON_SPAM:'beaconSpamActive',REACTIVE:'reactiveActive',RECOVER_SSID:'recoverSsidActive'};
  var anyOn=false;
  for(var cmd in map){
    var on=!!(data[map[cmd]]);
    if(on)anyOn=true;
    var btn=document.getElementById('btn-'+cmd);
    if(btn)btn.classList.toggle('on',on);
    var pill=document.getElementById('s-'+cmd);
    if(pill){pill.className='pill '+(on?'on':'off');pill.textContent=on?'ON':'OFF';}
  }
  var sp=document.getElementById('statusPill');
  sp.innerHTML=anyOn?'<span class="pill run">RUNNING</span>':'<span class="pill off">IDLE</span>';
  
  // Update target counts
  var selectedNetworks=data.targets?data.targets.filter(function(t){return t.selected;}).length:0;
  var selectedStations=data.stations?data.stations.filter(function(s){return s.selected;}).length:0;
  document.getElementById('target-networks').textContent=selectedNetworks;
  document.getElementById('target-stations').textContent=selectedStations;
}

function toggleSelAll(){var all=document.getElementById('selAllChk').checked;document.querySelectorAll('.lchk').forEach(function(c){c.checked=all;});}

function deleteAllLogs(){
  if(!confirm('Delete ALL logs?'))return;
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=DELETE_ALL_LOGS'}).then(function(){scheduleFetch();}).catch(function(e){console.error(e);});
}

function deleteSelectedLogs(){
  var chks=document.querySelectorAll('.lchk:checked');
  if(!chks.length)return;
  if(!confirm('Delete '+chks.length+' log(s)?'))return;
  var idx=[];chks.forEach(function(c){idx.push(c.getAttribute('data-index'));});
  fetch('/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd=DELETE_SELECTED_LOGS:'+idx.join(',')}).then(function(){scheduleFetch();}).catch(function(e){console.error(e);});
}

function tick(){
  var n=new Date();
  document.getElementById('tdisp').textContent=n.toLocaleTimeString();
  document.getElementById('ddisp').textContent=n.toLocaleDateString('en-US',{weekday:'short',year:'numeric',month:'short',day:'numeric'});
}

var lastTimeSync=0;
function syncTime(){
  if(Date.now()-lastTimeSync<25000)return;
  lastTimeSync=Date.now();
  var n=new Date(),p=function(x){return String(x).padStart(2,'0');};
  var ts=n.getFullYear()+'-'+p(n.getMonth()+1)+'-'+p(n.getDate())+' '+p(n.getHours())+':'+p(n.getMinutes())+':'+p(n.getSeconds());
  fetch('/sync_time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'time='+encodeURIComponent(ts)}).catch(function(e){console.error(e);});
}

function fetchMacs(){fetch('/mac').then(function(r){return r.json();}).then(function(d){document.getElementById('apMac').textContent=d.ap_mac;document.getElementById('staMac').textContent=d.sta_mac;}).catch(function(e){console.error(e);});}

function loadAPSettings(){
  fetch('/get-ap-settings').then(function(r){return r.json();}).then(function(d){
    document.getElementById('apNameInput').value=d.apName||'NETCIPHER';
    document.getElementById('apPasswordInput').value=d.apPassword||'deauther';
  }).catch(function(e){console.error('Error loading settings:',e);});
}

function saveAPSettings(evt){
  evt.preventDefault();
  var apName=document.getElementById('apNameInput').value;
  var apPassword=document.getElementById('apPasswordInput').value;
  if(!apName||!apPassword){alert('AP Name and Password are required');return;}
  if(apPassword.length<8){alert('Password must be at least 8 characters');return;}
  var statusDiv=document.getElementById('settingsStatus');
  statusDiv.style.display='block';
  statusDiv.textContent='Saving...';statusDiv.style.color='#880';
  fetch('/update-netcipher',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'apName='+encodeURIComponent(apName)+'&apPassword='+encodeURIComponent(apPassword)})
    .then(function(r){return r.text();})
    .then(function(result){
      statusDiv.textContent='Saved! AP restarting...';
      statusDiv.style.color='#4c4';
      setTimeout(function(){statusDiv.style.display='none';},3000);
    })
    .catch(function(e){
      console.error(e);
      statusDiv.textContent='Error saving settings';
      statusDiv.style.color='#c44';
    });
}

tick();setInterval(tick,1000);setInterval(syncTime,30000);
fetchStatus();fetchMacs();syncTime();
</script>
</body>
</html>
)rawliteral";

// HTML page for the display mirror (served as PROGMEM)
const char DISPLAY_MIRROR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NETCIPHER - Display Mirror</title>
    <style>
        body {
            background: #000;
            color: #0af;
            font-family: monospace;
            text-align: center;
            padding: 20px;
        }
        canvas {
            border: 2px solid #0af;
            box-shadow: 0 0 20px #0af;
            background: #000;
            image-rendering: crisp-edges;
        }
        .info {
            margin-top: 20px;
            font-size: 12px;
        }
        .status {
            color: #ff0;
        }
        .close-btn {
            margin-top: 12px;
            padding: 8px 20px;
            background: #0a0a2a;
            border: 2px solid #0af;
            color: #0af;
            cursor: pointer;
            font-family: monospace;
            font-size: 13px;
        }
        .close-btn:hover { background:#0af; color:#000; }
        /* D-Pad */
        .dpad-wrap { margin: 18px auto 0; display: inline-block; }
        .dpad-label { color:#0af; font-size:11px; letter-spacing:1px; margin-bottom:8px; }
        .dpad {
            display: grid;
            grid-template-columns: 54px 54px 54px;
            grid-template-rows: 54px 54px 54px;
            gap: 5px;
        }
        .dpad-btn {
            background: #0a1826;
            border: 2px solid #0af;
            color: #0af;
            font-size: 22px;
            cursor: pointer;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            user-select: none;
            transition: background .1s, transform .08s;
            -webkit-tap-highlight-color: transparent;
            touch-action: manipulation;
        }
        .dpad-btn:hover  { background: #0a2d45; }
        .dpad-btn:active { background: #0af; color: #000; transform: scale(0.93); }
        .dpad-empty { background: transparent; border: none; }
    </style>
</head>
<body>
    <h2>OLED LIVE MIRROR</h2>
    <canvas id="displayCanvas" width="128" height="64" style="width:256px; height:128px; image-rendering: crisp-edges;"></canvas>
    <div class="info">
        <span id="status" class="status">Updating...</span> 
        &nbsp;|&nbsp; 
        <span id="timestamp"></span>
    </div>
    <div class="dpad-wrap">
      <div class="dpad-label">VIRTUAL CONTROLS</div>
      <div class="dpad">
        <div class="dpad-empty"></div>
        <button class="dpad-btn" ontouchstart="sendBtn(event,'BTN_UP')" onclick="sendBtn(event,'BTN_UP')" title="UP">up</button>
        <div class="dpad-empty"></div>
        <button class="dpad-btn" ontouchstart="sendBtn(event,'BTN_BACK')" onclick="sendBtn(event,'BTN_BACK')" title="BACK">back</button>
        <div class="dpad-empty"></div>
        <button class="dpad-btn" ontouchstart="sendBtn(event,'BTN_OK')" onclick="sendBtn(event,'BTN_OK')" title="OK / SELECT">ok</button>
        <div class="dpad-empty"></div>
        <button class="dpad-btn" ontouchstart="sendBtn(event,'BTN_DOWN')" onclick="sendBtn(event,'BTN_DOWN')" title="DOWN">down</button>
        <div class="dpad-empty"></div>
      </div>
    </div>
    <div>
      <button class="close-btn" onclick="window.location.href='/'">CLOSE</button>
    </div>
    <script>
        function sendBtn(evt, cmd) {
            if (evt) evt.preventDefault();
            fetch('/command', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'cmd='+cmd});
        }
        const canvas = document.getElementById('displayCanvas');
        const ctx = canvas.getContext('2d');
        const width = 128;
        const height = 64;
        
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, width, height);

        function drawBuffer(buffer) {
            if (buffer.length !== 1024) {
                console.error('Invalid buffer size:', buffer.length);
                return;
            }
            const imgData = ctx.createImageData(width, height);
            for (let x = 0; x < width; x++) {
                for (let y = 0; y < height; y++) {
                    const page = y >> 3;
                    const bit = y & 0x07;
                    const byteIndex = x + page * width;
                    const pixel = (buffer[byteIndex] >> bit) & 0x01;
                    const idx = (y * width + x) * 4;
                    const val = pixel ? 255 : 0;
                    imgData.data[idx] = val;
                    imgData.data[idx+1] = val;
                    imgData.data[idx+2] = val;
                    imgData.data[idx+3] = 255;
                }
            }
            ctx.putImageData(imgData, 0, 0);
        }

        async function fetchBuffer() {
            try {
                const response = await fetch('/display_buffer');
                if (!response.ok) throw new Error('Network response error');
                const arrayBuffer = await response.arrayBuffer();
                const buffer = new Uint8Array(arrayBuffer);
                drawBuffer(buffer);
                document.getElementById('status').innerText = 'Live';
                document.getElementById('timestamp').innerText = new Date().toLocaleTimeString();
            } catch (err) {
                console.error('Fetch error:', err);
                document.getElementById('status').innerText = 'Error: ' + err.message;
            }
        }

        setInterval(fetchBuffer, 200);
        fetchBuffer();
    </script>
</body>
</html>
)rawliteral";

// Handler for the display mirror page
void handleDisplayMirror() {
    webServer.send(200, "text/html", DISPLAY_MIRROR_HTML);
}

// Handler for the raw display buffer
void handleDisplayBuffer() {
    // Get the current U8g2 buffer
    uint8_t* buffer = u8g2.getBufferPtr();
    
    // For SH1106 128x64, buffer size is always 1024 bytes
    // (128 columns * 8 pages)
    const size_t bufferSize = 1024;
    
    // Send as binary data using Stream API
    webServer.setContentLength(bufferSize);
    webServer.send(200, "application/octet-stream", "");
    webServer.sendContent((const char*)buffer, bufferSize);
}

// Call this from setupWebServer() to register the routes
void setupDisplayMirror() {
    webServer.on("/display_mirror", handleDisplayMirror);
    webServer.on("/display_buffer", handleDisplayBuffer);
}

#endif
