#ifndef WWW_INDEX_H
#define WWW_INDEX_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ATLAS-OS v2.3</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
    :root {
        --bg:#0a0a12;--glass:rgba(28,28,44,0.72);--glass-border:rgba(255,255,255,0.08);
        --accent:#00d4aa;--accent-dim:#00a080;--danger:#ff4757;--warning:#ffa502;
        --text:#e8e8f0;--text-dim:#8888a0;--text-muted:#555566;
        --chart-h:80px;
    }
    *{box-sizing:border-box;-webkit-tap-highlight-color:transparent;margin:0;padding:0}
    body{background:var(--bg);background-image:
        radial-gradient(ellipse at 15% 15%,rgba(0,212,170,.10) 0%,transparent 50%),
        radial-gradient(ellipse at 85% 85%,rgba(100,80,200,.08) 0%,transparent 50%);
        color:var(--text);font-family:'Inter',-apple-system,sans-serif;
        overflow:hidden;height:100vh}
    /* ── Desktop ── */
    #desktop{position:relative;width:100%;height:calc(100vh - 56px);padding:14px;
        overflow-y:auto;overflow-x:hidden;-webkit-overflow-scrolling:touch}
    /* ── Dock ── */
    .dock{position:fixed;bottom:10px;left:50%;transform:translateX(-50%);
        height:50px;background:rgba(18,18,30,.88);backdrop-filter:blur(24px);
        -webkit-backdrop-filter:blur(24px);border-radius:16px;
        border:1px solid var(--glass-border);display:flex;align-items:center;
        padding:0 8px;gap:3px;z-index:9999;
        box-shadow:0 8px 32px rgba(0,0,0,.45),inset 0 1px 0 rgba(255,255,255,.05)}
    .dock-item{width:42px;height:42px;border-radius:11px;display:flex;align-items:center;
        justify-content:center;cursor:pointer;font-size:20px;color:var(--text-dim);
        transition:all .18s cubic-bezier(.34,1.56,.64,1)}
    .dock-item:hover{transform:translateY(-5px) scale(1.1);color:var(--text);background:rgba(255,255,255,.07)}
    .dock-item.active{color:var(--accent);background:rgba(0,212,170,.15)}
    /* ── Glass Window ── */
    .window{position:absolute;background:var(--glass);backdrop-filter:blur(28px) saturate(170%);
        -webkit-backdrop-filter:blur(28px) saturate(170%);border:1px solid var(--glass-border);
        border-radius:14px;box-shadow:0 14px 44px rgba(0,0,0,.4),inset 0 1px 0 rgba(255,255,255,.06);
        display:flex;flex-direction:column;overflow:hidden;
        animation:winIn .3s cubic-bezier(.34,1.56,.64,1)}
    @keyframes winIn{from{opacity:0;transform:scale(.93) translateY(8px)}to{opacity:1;transform:scale(1) translateY(0)}}
    .win-header{background:rgba(255,255,255,.03);padding:11px 14px;cursor:grab;
        display:flex;justify-content:space-between;align-items:center;
        border-bottom:1px solid var(--glass-border);user-select:none;-webkit-user-select:none;flex-shrink:0}
    .win-title{font-size:12px;font-weight:600;color:var(--text);
        display:flex;align-items:center;gap:7px;letter-spacing:.01em}
    .win-icon{font-size:14px;width:22px;height:22px;border-radius:5px;
        display:flex;align-items:center;justify-content:center}
    .win-controls{display:flex;gap:6px}
    .win-btn{width:12px;height:12px;border-radius:50%;cursor:pointer;flex-shrink:0;transition:opacity .15s}
    .win-btn.close{background:#ff5f57}.win-btn.min{background:#febc2e}.win-btn.max{background:#28c840}
    .win-content{padding:14px;overflow-y:auto;flex:1;min-height:0}
    .win-content::-webkit-scrollbar{width:5px}
    .win-content::-webkit-scrollbar-thumb{background:rgba(255,255,255,.14);border-radius:3px}
    /* ── Cards ── */
    .card{background:rgba(255,255,255,.035);border-radius:12px;padding:13px;
        margin-bottom:10px;border:1px solid var(--glass-border)}
    .card-title{font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
        letter-spacing:.8px;margin-bottom:9px;display:flex;align-items:center;gap:5px}
    .card-2col{display:grid;grid-template-columns:1fr 1fr;gap:9px}
    .card-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(100px,1fr));gap:7px}
    /* ── Sensor Tile ── */
    .sensor{background:rgba(0,0,0,.22);border-radius:10px;padding:11px 10px;
        position:relative;overflow:hidden;cursor:default}
    .sensor::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;border-radius:10px 10px 0 0}
    .sensor.good::before{background:var(--accent)}.sensor.warn::before{background:var(--warning)}.sensor.danger::before{background:var(--danger)}
    .sensor-icon{font-size:16px;margin-bottom:3px;line-height:1}
    .sensor-label{font-size:9px;color:var(--text-muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
    .sensor-value{font-size:17px;font-weight:700;color:var(--text);line-height:1.2;margin:2px 0}
    .sensor-unit{font-size:9px;color:var(--text-dim)}
    /* ── Human-centric gauges ── */
    .gauge-wrap{background:rgba(0,0,0,.24);border:1px solid var(--glass-border);border-radius:12px;padding:12px 13px}
    .gauge-h{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:8px}
    .gauge-label{font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.7px;font-weight:600}
    .gauge-val{font-size:14px;font-weight:800;font-family:'JetBrains Mono',monospace}
    /* outer track (grey shell) — marks go here, no overflow:hidden */
    .gauge-outer{position:relative;height:18px;display:flex;align-items:center}
    .gauge-track{height:10px;border-radius:999px;background:rgba(255,255,255,.07);width:100%;
        position:relative;overflow:hidden}
    /* coloured progress fill — ONLY to value */
    .gauge-fill{position:absolute;left:0;top:0;bottom:0;border-radius:999px;
        transition:width .6s cubic-bezier(.4,0,.2,1)}
    /* threshold tick marks — sit in .gauge-outer, above track */
    .gauge-mark{position:absolute;top:0;bottom:0;width:2px;border-radius:1px;
        background:rgba(255,255,255,.8);box-shadow:0 0 5px rgba(255,255,255,.7)}
    .gauge-foot{display:flex;justify-content:space-between;font-size:9px;color:var(--text-muted);margin-top:4px}
    .hero-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
    .dash-mix{display:grid;grid-template-columns:1.1fr .9fr;gap:9px}
    /* ── Analogue clock — fixed ── */
    .analogue-clock{width:160px;height:160px;margin:0 auto;border-radius:50%;
        position:relative;overflow:hidden;
        background:radial-gradient(circle at 40% 30%, rgba(255,255,255,.13), rgba(0,0,0,.55));
        border:2px solid rgba(255,255,255,.1);
        box-shadow:inset 0 0 30px rgba(0,0,0,.5),0 8px 24px rgba(0,0,0,.5)}
    /* marks: placed at top of clock, rotate around clock center */
    .clk-mark{position:absolute;width:2px;height:8px;background:rgba(255,255,255,.45);
        left:calc(50% - 1px);top:calc(50% - 78px);
        transform-origin:1px 78px;border-radius:1px}
    .clk-mark.major{height:12px;top:calc(50% - 78px);width:3px;left:calc(50% - 1.5px);
        background:rgba(255,255,255,.75);transform-origin:1.5px 78px}
    .clk-hand{position:absolute;left:50%;top:50%;transform-origin:50% 100%;border-radius:4px 4px 2px 2px}
    .clk-hour{width:5px;height:42px;margin-left:-2.5px;margin-top:-42px;
        background:linear-gradient(to top,rgba(255,255,255,.9),rgba(255,255,255,.5))}
    .clk-min{width:3px;height:58px;margin-left:-1.5px;margin-top:-58px;
        background:linear-gradient(to top,#93c5fd,rgba(147,197,253,.5))}
    .clk-sec{width:2px;height:66px;margin-left:-1px;margin-top:-54px;
        background:#f97316;box-shadow:0 0 8px rgba(249,115,22,.8)}
    .clk-tail{position:absolute;width:2px;height:12px;left:calc(50% - 1px);top:50%;
        background:#f97316;transform-origin:1px 0;border-radius:0 0 2px 2px}
    .clk-dot{position:absolute;left:50%;top:50%;width:9px;height:9px;
        margin:-4.5px 0 0 -4.5px;border-radius:50%;
        background:#fff;box-shadow:0 0 6px rgba(255,255,255,.6)}
    .clk-rim{position:absolute;inset:3px;border-radius:50%;
        border:1px solid rgba(255,255,255,.07);pointer-events:none}
    .kv-table{width:100%;border-collapse:collapse;font-size:11px}
    .kv-table td{padding:6px 4px;border-bottom:1px solid rgba(255,255,255,.06)}
    .kv-k{color:var(--text-dim)}
    .kv-v{text-align:right;color:var(--text);font-weight:600;font-family:'JetBrains Mono',monospace}
    /* ── Battery donut ── */
    .bat-donut{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px}
    .bat-donut svg{filter:drop-shadow(0 0 8px rgba(0,212,170,.3))}
    .bat-center{position:absolute;text-align:center;pointer-events:none}
    .bat-pct{font-size:24px;font-weight:900;color:#fff;line-height:1}
    .bat-sub{font-size:10px;color:rgba(255,255,255,.45);margin-top:1px}
    .bat-row{display:flex;gap:12px;justify-content:center;flex-wrap:wrap}
    .bat-kv{display:flex;flex-direction:column;align-items:center;gap:1px}
    .bat-kv span:first-child{font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px}
    .bat-kv span:last-child{font-size:13px;font-weight:700;color:var(--text);font-family:'JetBrains Mono',monospace}
    /* ── AQI Dial ── */
    .aqi-wrap{display:flex;flex-direction:column;align-items:center;gap:4px}
    .aqi-label{font-size:11px;font-weight:700;letter-spacing:.5px}
    /* ── Wind rose ── */
    .wind-wrap{display:flex;flex-direction:column;align-items:center;gap:6px}
    .wind-info{display:flex;gap:16px;justify-content:center}
    .wind-kv{display:flex;flex-direction:column;align-items:center}
    .wind-kv span:first-child{font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px}
    .wind-kv span:last-child{font-size:14px;font-weight:700;color:var(--text)}
    /* ── Pressure trend ── */
    .press-card{display:flex;align-items:center;gap:12px;padding:10px;
        background:rgba(0,0,0,.22);border-radius:10px;border:1px solid var(--glass-border)}
    .press-arrow{font-size:36px;line-height:1}
    .press-info{flex:1}
    .press-val{font-size:22px;font-weight:800;color:var(--text)}
    .press-forecast{font-size:11px;color:var(--text-dim);margin-top:2px}
    .press-delta{font-size:10px;margin-top:1px}
    /* ── Fault matrix ── */
    .fault-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(88px,1fr));gap:5px}
    .fault-item{display:flex;align-items:center;gap:5px;padding:5px 7px;
        background:rgba(0,0,0,.20);border-radius:7px;font-size:10px;color:var(--text-dim)}
    .fault-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0}
    .fault-dot.ok{background:#22c55e;box-shadow:0 0 5px #22c55e88}
    .fault-dot.bad{background:#ef4444;box-shadow:0 0 5px #ef444488;animation:pulse-bad .8s infinite alternate}
    .fault-dot.na{background:#475569}
    @keyframes pulse-bad{from{opacity:1}to{opacity:.4}}
    /* ── Spark-in-tile ── */
    .sensor canvas.spark{position:absolute;bottom:0;left:0;right:0;height:28px;opacity:.45;border-radius:0 0 10px 10px}
    /* ── I2C scanner ── */
    .i2c-addr{display:inline-flex;align-items:center;gap:4px;background:rgba(255,255,255,.07);
        border-radius:6px;padding:2px 8px;margin:2px;white-space:nowrap;position:relative;cursor:default}
    .i2c-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0}
    .i2c-dot.ok{background:#22c55e;box-shadow:0 0 5px #22c55e88}
    .i2c-dot.bad{background:#ef4444;box-shadow:0 0 5px #ef444488}
    .i2c-dot.unknown{background:#f59e0b;box-shadow:0 0 5px #f59e0b88}
    .i2c-dot.unassigned{background:#475569}
    .i2c-qmark{width:14px;height:14px;border-radius:50%;background:rgba(255,255,255,.12);
        color:rgba(255,255,255,.55);font-size:9px;font-weight:700;display:inline-flex;
        align-items:center;justify-content:center;cursor:help;flex-shrink:0}
    .i2c-tt{display:none;position:absolute;bottom:calc(100% + 6px);left:50%;transform:translateX(-50%);
        background:rgba(12,12,24,.97);border:1px solid rgba(255,255,255,.14);border-radius:8px;
        padding:7px 11px;z-index:9000;min-width:160px;max-width:260px;pointer-events:none;
        box-shadow:0 8px 24px rgba(0,0,0,.6);backdrop-filter:blur(12px)}
    .i2c-tt::after{content:'';position:absolute;top:100%;left:50%;transform:translateX(-50%);
        border:6px solid transparent;border-top-color:rgba(255,255,255,.14)}
    .i2c-qmark:hover .i2c-tt,.i2c-addr:hover .i2c-tt{display:block}
    .i2c-tt-name{font-size:12px;font-weight:700;color:#e2e8f0;margin-bottom:3px}
    .i2c-tt-addr{font-size:10px;color:#64748b;font-family:'JetBrains Mono',monospace}
    .i2c-tt-status{font-size:10px;margin-top:4px;font-weight:600}
    .i2c-tt-desc{font-size:10px;color:#94a3b8;margin-top:2px;line-height:1.4}
    /* ── Mode buttons active ── */
    .mode-btn{transition:all .2s}
    .mode-btn.mode-active{background:rgba(0,212,170,.22)!important;border-color:rgba(0,212,170,.6)!important;
        color:var(--accent)!important;box-shadow:0 0 12px rgba(0,212,170,.25)}
    /* ── Discovery report ── */
    .disc-item{padding:8px 10px;border-radius:8px;margin-bottom:6px;font-size:11px;
        background:rgba(0,0,0,.2);border:1px solid rgba(255,255,255,.06)}
    .disc-found{border-left:3px solid #22c55e}
    .disc-miss{border-left:3px solid #ef4444;opacity:.7}
    /* ── Sensor List rows ── */
    .srow{display:flex;justify-content:space-between;align-items:center;
        padding:6px 0;border-bottom:1px solid rgba(255,255,255,.05);font-size:11px;
        cursor:pointer;transition:background .1s;border-radius:4px}
    .srow:hover{background:rgba(255,255,255,.04)}
    .srow span:first-child{color:var(--text-dim);flex:1;padding-right:8px}
    .srow span:last-child{color:var(--text);font-weight:600;white-space:nowrap;font-family:'JetBrains Mono',monospace}
    /* ── Sparkline chart ── */
    .chartbox{background:rgba(0,0,0,.25);border-radius:8px;padding:8px 10px;margin-top:6px;
        position:relative;height:var(--chart-h);overflow:hidden}
    .chartbox canvas{position:absolute;inset:0;width:100%;height:100%}
    .chartbox .chart-label{font-size:9px;color:var(--text-muted);position:absolute;top:5px;left:8px;z-index:1}
    .chartbox .chart-cur{font-size:13px;font-weight:700;color:var(--accent);
        position:absolute;bottom:5px;right:8px;z-index:1;font-family:'JetBrains Mono',monospace}
    /* ── Terminal ── */
    #log-terminal{
        background:rgba(0,0,0,.6);color:#00ff88;
        font-family:'JetBrains Mono','Fira Code',monospace;
        font-size:11px;line-height:1.55;
        padding:12px;border-radius:10px;
        white-space:pre-wrap;word-break:break-all;
        overflow-y:auto;
        min-height:320px;
        max-height:min(70vh,600px);
        border:1px solid rgba(0,255,136,.12);
        cursor:text;
        /* allow text selection for copy */
        user-select:text;-webkit-user-select:text
    }
    #log-terminal ::selection{background:rgba(0,212,170,.35);color:#fff}
    .log-toolbar{display:flex;gap:6px;margin-bottom:8px;align-items:center}
    .log-search{flex:1;background:rgba(0,0,0,.35);color:var(--text);
        border:1px solid var(--glass-border);border-radius:7px;
        padding:5px 10px;font-family:'JetBrains Mono',monospace;font-size:11px;outline:none}
    .log-search:focus{border-color:var(--accent-dim)}
    /* ── Buttons ── */
    .btn{background:rgba(255,255,255,.06);color:var(--text);
        border:1px solid var(--glass-border);padding:9px 14px;border-radius:9px;
        margin:2px;font-family:inherit;font-size:12px;cursor:pointer;
        transition:all .18s;display:inline-flex;align-items:center;gap:5px}
    .btn:hover{background:rgba(255,255,255,.11);transform:translateY(-1px)}
    .btn:active{transform:translateY(0)}
    .btn.danger{background:rgba(255,71,87,.14);border-color:rgba(255,71,87,.28);color:#ff6b7a}
    .btn.danger:hover{background:rgba(255,71,87,.24)}
    .btn.accent{background:rgba(0,212,170,.14);border-color:rgba(0,212,170,.28);color:var(--accent)}
    .btn.accent:hover{background:rgba(0,212,170,.24)}
    .btn-group{display:flex;flex-wrap:wrap;gap:3px;margin:7px 0}
    /* ── Input ── */
    input,select{background:rgba(0,0,0,.3);color:var(--text);border:1px solid var(--glass-border);
        padding:9px 11px;border-radius:8px;font-family:inherit;font-size:12px;outline:none;width:100%}
    input:focus,select:focus{border-color:var(--accent-dim)}
    /* ── Status bar ── */
    .status-bar{position:fixed;top:10px;right:12px;display:flex;gap:7px;z-index:200}
    .status-pill{background:rgba(18,18,30,.8);backdrop-filter:blur(10px);
        padding:5px 11px;border-radius:18px;font-size:10px;
        display:flex;align-items:center;gap:5px;border:1px solid var(--glass-border)}
    .status-dot{width:6px;height:6px;border-radius:50%;background:var(--accent)}
    .status-dot.offline{background:var(--danger)}
    #ota-prog{text-align:center;margin-top:8px;font-size:11px;color:var(--accent)}
    /* ── Copy toast ── */
    .toast{position:fixed;bottom:70px;left:50%;transform:translateX(-50%) translateY(8px);
        background:rgba(0,212,170,.18);border:1px solid rgba(0,212,170,.35);color:var(--accent);
        font-size:11px;padding:6px 14px;border-radius:20px;opacity:0;
        transition:all .25s;z-index:9999;pointer-events:none}
    .toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
    /* ── Mobile ── */
    @media(max-width:600px){
        #desktop{padding:8px;padding-bottom:64px}
        .window{min-width:calc(100vw - 16px) !important}
        .card-grid{grid-template-columns:repeat(2,1fr)}
        .card-2col{grid-template-columns:1fr}
    }
</style>
</head>
<body>
<div class="status-bar">
    <div class="status-pill"><div class="status-dot" id="wifiDot"></div><span id="wifiStatus">…</span></div>
    <div class="status-pill">🌡️ <span id="quickTemp">--</span>°C &nbsp;💧<span id="quickHum">--</span>%</div>
    <div class="status-pill" id="langPill" style="cursor:pointer;gap:5px;transition:background .2s" title="Toggle language PL/EN" onclick="setLang(LANG==='en'?'pl':'en')">
        <span id="langFlag" style="font-size:16px;line-height:1"></span>
        <span id="langLbl" style="font-size:10px;font-weight:700;letter-spacing:.3px"></span>
    </div>
    <div class="status-pill"><span id="clock">--:--</span></div>
</div>
<div id="desktop"></div>
<div class="dock" id="dock"></div>
<div class="toast" id="toast">Copied!</div>

<script>
// ============================================================
// ATLAS-OS v2.3 — glass window desktop + charts + wide logs
// ============================================================
const DOCK_BTNS = [
    {id:'dash',    icon:'◉',   title:'Dashboard'},
    {id:'medical', icon:'🏥',  title:'Medical'},
    {id:'space',   icon:'🌌',  title:'Space Weather'},
    {id:'environ', icon:'🌿',  title:'Environment'},
    {id:'spectr',  icon:'🌈',  title:'Spectrometer'},
    
    {id:'imu',     icon:'🔭',  title:'IMU & Mag'},
    {id:'sensors', icon:'📡',  title:'All Sensors'},
    {id:'charts',  icon:'📈',  title:'Charts'},
    {id:'sys',     icon:'⚙️', title:'System'},
    {id:'log',     icon:'📜',  title:'Logs'},
];
const ICON_COLORS = {
    '◉':'#00d4aa','🏥':'#fb7185','🌌':'#818cf8','🌿':'#4ade80',
    '🌈':'#f472b6','🌡️':'#f97316','🔭':'#60a5fa','📡':'#a78bfa',
    '📈':'#f59e0b','⚙️':'#fbbf24','📜':'#94a3b8'
};

// ── i18n language ──
let LANG = localStorage.getItem('atlas_lang') || 'en';
const T = {
    en: {
        dashboard:'Dashboard', medical:'Medical', space:'Space Weather',
        environ:'Environment', spectr:'Spectrometer', 
        imu:'IMU & Mag', sensors:'All Sensors', charts:'Charts',
        sys:'System', log:'Logs',
        feels:'Feels Like', temp:'Temperature', hum:'Humidity',
        press:'Pressure', dew:'Dew Point', cloud:'Cloud Base',
        battery:'Battery', solar:'Solar', iaq:'Air Quality', co2:'CO₂',
        uvi:'UV Index', radiation:'Radiation', wind:'Wind',
        migraine:'Migraine Risk', rheum:'Rheumatic Risk', sinus:'Sinus Risk',
        utci:'Thermal Comfort', radon:'Radon Risk', lung:'Lung Deposition',
        aurora:'Aurora Probability', kindex:'K-Index', ozone:'Ozone',
        forbush:'Forbush Decrease', ssc:'CME / SSC', skyq:'Sky Condition',
        aod:'Aerosol Depth', ghi:'Solar Irradiance',
        biometeo:'Biometeo Alert', bpi:'Baro Pain Index',
    },
    pl: {
        dashboard:'Panel Główny', medical:'Medyczny', space:'Pogoda Kosmiczna',
        environ:'Środowisko', spectr:'Spektrometr', 
        imu:'IMU & Mag', sensors:'Wszystkie Czujniki', charts:'Wykresy',
        sys:'System', log:'Logi',
        feels:'Odczuwalna', temp:'Temperatura', hum:'Wilgotność',
        press:'Ciśnienie', dew:'Punkt Rosy', cloud:'Podstawa Chmur',
        battery:'Bateria', solar:'Solar', iaq:'Jakość Powietrza', co2:'CO₂',
        uvi:'Indeks UV', radiation:'Promieniowanie', wind:'Wiatr',
        migraine:'Ryzyko Migreny', rheum:'Ryzyko Reumatyczne', sinus:'Ryzyko Zatok',
        utci:'Komfort Cieplny', radon:'Ryzyko Radonu', lung:'Depozycja Płucna',
        aurora:'Prawdopodob. Zorzy', kindex:'Indeks K', ozone:'Ozon',
        forbush:'Spadek Forbusha', ssc:'CME / SSC', skyq:'Stan Nieba',
        aod:'Głębokość Aerozol.', ghi:'Nasl. Słoneczne',
        biometeo:'Alert Biometeo', bpi:'Barometryczny Ból',
    }
};
function t(key){ return (T[LANG]||T.en)[key] || key; }
function setLang(l){ LANG=l; localStorage.setItem('atlas_lang',l); location.reload(); }

let zIdx = 100;
const desktop = document.getElementById('desktop');
const dock    = document.getElementById('dock');

const $  = id => document.getElementById(id);
const el = (tag,cls,html) => { const e=document.createElement(tag); if(cls) e.className=cls; if(html) e.innerHTML=html; return e; };

// ── Clock ──
const updateClock = () => $('clock').textContent = new Date().toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});
setInterval(updateClock,1000); updateClock();
setInterval(tickDashClock,1000);

// ── WiFi + quick readings ──
function pollStatus(){
    fetch('/wifi').then(r=>r.text()).then(t=>{
        const ok = t.includes('OK');
        $('wifiDot').className = 'status-dot'+(ok?'':' offline');
        $('wifiStatus').textContent = ok?'Online':'Offline';
    }).catch(()=>{ $('wifiDot').className='status-dot offline'; $('wifiStatus').textContent='Offline'; });
    fetch('/api').then(r=>r.json()).then(d=>{
        const s = d.sensors||{};
        $('quickTemp').textContent = s.SHT45_Temp!=null ? (+s.SHT45_Temp).toFixed(1) : '--';
        $('quickHum').textContent  = s.SHT45_Hum !=null ? (+s.SHT45_Hum).toFixed(0)  : '--';
    }).catch(()=>{});
}
setInterval(pollStatus,10000); pollStatus();

// ── Toast copy notification ──
function showToast(msg='Copied!'){
    const t=$('toast'); t.textContent=msg; t.classList.add('show');
    setTimeout(()=>t.classList.remove('show'),1800);
}

// ── Dock ──
DOCK_BTNS.forEach((b,i)=>{
    const d = el('div','dock-item',b.icon);
    d.title=b.title; d.dataset.id=b.id;
    d.onclick=()=>loadSection(b.id);
    dock.appendChild(d);
});
function setActiveBtn(id){
    dock.querySelectorAll('.dock-item').forEach(d=>d.classList.toggle('active',d.dataset.id===id));
}

// ── Window factory ──
function createWindow(id, title, icon, x, y, content, w=340) {
    if($(id)){ $(id).style.display='flex'; focusWin(id); return $(id); }
    const col = ICON_COLORS[icon]||'#888';
    const win = el('div','window');
    win.id=id;
    const vw=window.innerWidth, vh=window.innerHeight;
    win.style.cssText = `left:${Math.min(x,vw-w-10)}px;top:${Math.min(y,vh-300)}px;width:${Math.min(w,vw-16)}px;z-index:${++zIdx};max-height:${vh-80}px`;
    win.innerHTML = `
        <div class="win-header" onmousedown="dragStart(event,'${id}')" ontouchstart="dragStart(event,'${id}')">
            <div class="win-title">
                <span class="win-icon" style="background:${col}22;color:${col}">${icon}</span>${title}
            </div>
            <div class="win-controls">
                <div class="win-btn min" onclick="minWin('${id}')"></div>
                <div class="win-btn max" onclick="maxWin('${id}')"></div>
                <div class="win-btn close" onclick="closeWin('${id}')"></div>
            </div>
        </div>
        <div class="win-content">${content}</div>`;
    desktop.appendChild(win);
    win.addEventListener('mousedown',()=>focusWin(id));
    return win;
}
function focusWin(id){ const w=$(id); if(w) w.style.zIndex=++zIdx; }
function minWin(id){ const w=$(id); if(w) w.style.display='none'; }
function maxWin(id){
    const w=$(id); if(!w) return;
    if(w._maximized){ Object.assign(w.style,w._prev||{}); w._maximized=false; }
    else {
        w._prev={left:w.style.left,top:w.style.top,width:w.style.width,maxHeight:w.style.maxHeight};
        Object.assign(w.style,{left:'8px',top:'8px',width:(window.innerWidth-16)+'px',maxHeight:(window.innerHeight-72)+'px'});
        w._maximized=true;
    }
}
function closeWin(id){ const w=$(id); if(w) w.remove(); }

// ── Drag ──
let _dw=null,_do={x:0,y:0};
function dragStart(e,id){
    e.preventDefault(); _dw=$(id); focusWin(id);
    const p=e.touches?e.touches[0]:e, r=_dw.getBoundingClientRect();
    _do={x:p.clientX-r.left,y:p.clientY-r.top};
    document.addEventListener('mousemove',dragMove);
    document.addEventListener('mouseup',dragEnd);
    document.addEventListener('touchmove',dragMove,{passive:false});
    document.addEventListener('touchend',dragEnd);
}
function dragMove(e){ if(!_dw) return; e.preventDefault();
    const p=e.touches?e.touches[0]:e;
    _dw.style.left=(p.clientX-_do.x)+'px';
    _dw.style.top =(p.clientY-_do.y)+'px';
}
function dragEnd(){ _dw=null;
    ['mousemove','mouseup','touchmove','touchend'].forEach(ev=>document.removeEventListener(ev,ev==='mousemove'||ev==='touchmove'?dragMove:dragEnd));
}

// ── Notify ──
function notify(msg,ok=true){
    const n=el('div','card');
    n.style.cssText=`position:fixed;top:56px;right:14px;z-index:10001;padding:9px 16px;font-size:12px;
        background:rgba(${ok?'0,212,170':'255,71,87'},.18);border-color:rgba(${ok?'0,212,170':'255,71,87'},.35);
        color:${ok?'var(--accent)':'var(--danger)'}`;
    n.textContent=msg; document.body.appendChild(n);
    setTimeout(()=>n.remove(),2200);
}

// ── API cmd ──
function cmd(action,extras=''){
    fetch('/cmd?action='+action+extras).then(r=>r.text()).then(()=>notify('✓ '+action)).catch(()=>notify('✗ failed',false));
}

// ── Init language pill ──
(function initLangPill(){
    const f=$('langFlag'), l=$('langLbl'), p=$('langPill');
    if(LANG==='pl'){
        if(f) f.textContent='🇵🇱';
        if(l) l.textContent='PL';
        if(p) p.style.borderColor='rgba(220,36,31,.35)';
    } else {
        if(f) f.textContent='🇬🇧';
        if(l) l.textContent='EN';
        if(p) p.style.borderColor='rgba(0,82,180,.35)';
    }
})();

// ── Sensor tile builder ──
function sensorTile(icon,label,val,unit,status='good'){
    return `<div class="sensor ${status}">
        <div class="sensor-icon">${icon}</div>
        <div class="sensor-label">${label}</div>
        <div class="sensor-value">${val??'--'}</div>
        <div class="sensor-unit">${unit}</div>
    </div>`;
}
function fv(v,dp=1){ return (v!=null&&v!==undefined)?(+v).toFixed(dp):'--'; }
function fvi(v){ return (v!=null&&v!==undefined)?Math.round(+v):'--'; }
function status_iaq(v){ return v>200?'danger':v>100?'warn':'good'; }
function status_co2(v){ return v>2000?'danger':v>1000?'warn':'good'; }
function status_pm(v){  return v>55?'danger':v>35?'warn':'good'; }
function status_rad(v){ return v>0.5?'danger':v>0.2?'warn':'good'; }
function status_num(v,warn,danger){ return (v>=danger)?'danger':(v>=warn)?'warn':'good'; }

function clamp(v,min,max){ return Math.max(min,Math.min(max,v)); }
function gauge(label,val,min,max,warn,danger,unit=''){
    const n = (val!=null && val!==undefined) ? +val : NaN;
    const safe = Number.isFinite(n);
    const cur = safe ? clamp(n,min,max) : min;
    const pct = ((cur-min)/(max-min))*100;
    const warnPct = ((warn-min)/(max-min))*100;
    const dangerPct = ((danger-min)/(max-min))*100;
    // fill colour = position-based: green → amber → red
    const col = !safe ? '#475569' : (n>=danger?'#ef4444':n>=warn?'#f59e0b':'#22c55e');
    // gradient on fill: always green→col so it looks smooth
    const fillGrad = !safe ? '#475569' : (n>=danger
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 45%,#ef4444)'
        : n>=warn
        ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)');
    const dispVal = safe ? (n<10&&n%1!==0?n.toFixed(1):n<100?n.toFixed(1):Math.round(n)) : '--';
    return `<div class="gauge-wrap">
        <div class="gauge-h">
            <div class="gauge-label">${label}</div>
            <div class="gauge-val" style="color:${col}">${dispVal}${unit?' <span style="font-size:10px;font-weight:400;color:var(--text-dim)">'+unit+'</span>':''}</div>
        </div>
        <div class="gauge-outer">
            <div class="gauge-track" style="flex:1">
                <div class="gauge-fill" style="width:${pct}%;background:${fillGrad}"></div>
            </div>
            <div class="gauge-mark" style="left:${warnPct}%;top:2px;bottom:2px" title="Warn: ${warn}${unit}"></div>
            <div class="gauge-mark" style="left:${dangerPct}%;top:0;bottom:0;background:#ef4444;box-shadow:0 0 6px #ef444488" title="Danger: ${danger}${unit}"></div>
        </div>
        <div class="gauge-foot"><span>${min}${unit}</span><span>${max}${unit}</span></div>
    </div>`;
}

function analogClockHTML(){
    let marks='';
    for(let i=0;i<60;i++){
        const isMaj=(i%5===0);
        marks+=`<div class="clk-mark${isMaj?' major':''}" style="transform:rotate(${i*6}deg)"></div>`;
    }
    return `<div class="analogue-clock" id="dashClock">
        <div class="clk-rim"></div>
        ${marks}
        <div class="clk-hand clk-hour" id="clkHour"></div>
        <div class="clk-hand clk-min"  id="clkMin"></div>
        <div class="clk-hand clk-sec"  id="clkSec"></div>
        <div class="clk-tail"          id="clkTail"></div>
        <div class="clk-dot"></div>
    </div>`;
}
function tickDashClock(){
    const now = new Date();
    const sec = now.getSeconds(), ms = now.getMilliseconds();
    const min = now.getMinutes();
    const hr  = now.getHours()%12;
    const hEl=$('clkHour'), mEl=$('clkMin'), sEl=$('clkSec'), tEl=$('clkTail');
    if(!hEl||!mEl||!sEl) return;
    // smooth second hand
    const sDeg = sec*6 + ms*0.006;
    hEl.style.transform=`rotate(${hr*30 + min*0.5 + sec*0.00833}deg)`;
    mEl.style.transform=`rotate(${min*6 + sec*0.1}deg)`;
    sEl.style.transform=`rotate(${sDeg}deg)`;
    if(tEl) tEl.style.transform=`rotate(${sDeg}deg)`;
}

// ══════════════════════════════════════════════════════
// BATTERY RING DONUT  — SVG arc, no libs
// ══════════════════════════════════════════════════════
function batteryDonut(soc, volt, solar_mw, esp_mw, drain_rate, tte_min){
    const R=54, CX=64, CY=64, stroke=10;
    const circ=2*Math.PI*R;
    const safe=Number.isFinite(+soc)&&+soc>=0;
    const pct=safe?Math.min(100,Math.max(0,+soc)):0;
    const off=circ*(1-pct/100);
    const col=pct>60?'#22c55e':pct>25?'#f59e0b':'#ef4444';
    const glow=pct>60?'#22c55e55':pct>25?'#f59e0b55':'#ef444455';
    const drStr=Number.isFinite(+drain_rate)?(+drain_rate>0?'+'+fv(drain_rate,1):fv(drain_rate,1))+'%/h':'--';
    const tteStr=Number.isFinite(+tte_min)&&+tte_min>0?(+tte_min/60).toFixed(1)+'h':'--';
    return `<div class="bat-donut">
        <div style="position:relative;width:128px;height:128px">
            <svg width="128" height="128" viewBox="0 0 128 128">
                <defs><filter id="gf"><feGaussianBlur stdDeviation="2"/></filter></defs>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="rgba(255,255,255,.07)" stroke-width="${stroke}"/>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="${glow}" stroke-width="${stroke+6}" filter="url(#gf)" stroke-dasharray="${circ*pct/100} ${circ}" stroke-dashoffset="${circ/4}" stroke-linecap="round"/>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="${col}" stroke-width="${stroke}" stroke-dasharray="${circ*pct/100} ${circ}" stroke-dashoffset="${circ/4}" stroke-linecap="round" style="transition:stroke-dasharray .8s ease"/>
            </svg>
            <div class="bat-center" style="top:0;left:0;right:0;bottom:0;display:flex;flex-direction:column;align-items:center;justify-content:center">
                <div class="bat-pct" style="color:${col}">${safe?Math.round(pct):'--%'}<span style="font-size:13px">%</span></div>
                <div class="bat-sub">${Number.isFinite(+volt)?fv(volt,2)+' V':'-- V'}</div>
            </div>
        </div>
        <div class="bat-row">
            <div class="bat-kv"><span>Solar</span><span style="color:#fbbf24">${Number.isFinite(+solar_mw)?fv(solar_mw,0)+' mW':'--'}</span></div>
            <div class="bat-kv"><span>Load</span><span style="color:#f87171">${Number.isFinite(+esp_mw)?fv(esp_mw,0)+' mW':'--'}</span></div>
            <div class="bat-kv"><span>Rate</span><span style="color:${(+drain_rate||0)>0?'#4ade80':'#fb923c'}">${drStr}</span></div>
            <div class="bat-kv"><span>ETA</span><span>${tteStr}</span></div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// AQI SEMI-CIRCLE SPEEDOMETER  — canvas, WHO bands
// ══════════════════════════════════════════════════════
function drawAqiDial(canvasId, iaq){
    const cvs=$(canvasId); if(!cvs) return;
    const W=200, H=120;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    const cx=W/2, cy=H-10, r=88;
    const bands=[
        {lo:0,  hi:50,  col:'#22c55e', lbl:'Excellent'},
        {lo:50, hi:100, col:'#84cc16', lbl:'Good'},
        {lo:100,hi:150, col:'#f59e0b', lbl:'Moderate'},
        {lo:150,hi:200, col:'#f97316', lbl:'Poor'},
        {lo:200,hi:250, col:'#ef4444', lbl:'Bad'},
        {lo:250,hi:350, col:'#9333ea', lbl:'Hazardous'},
    ];
    const MAX=350;
    const toAng=(v)=>Math.PI+(Math.PI*Math.min(v,MAX)/MAX);
    // draw bands
    bands.forEach(b=>{
        ctx.beginPath();
        ctx.arc(cx,cy,r,toAng(b.lo),toAng(b.hi));
        ctx.lineWidth=16; ctx.strokeStyle=b.col+'cc'; ctx.stroke();
    });
    // track bg
    ctx.beginPath(); ctx.arc(cx,cy,r-22,Math.PI,2*Math.PI);
    ctx.lineWidth=1; ctx.strokeStyle='rgba(255,255,255,.08)'; ctx.stroke();
    // needle
    const val=Number.isFinite(+iaq)?Math.min(+iaq,MAX):0;
    const ang=toAng(val);
    const nx=cx+Math.cos(ang)*(r-10), ny=cy+Math.sin(ang)*(r-10);
    ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(nx,ny);
    ctx.lineWidth=3; ctx.strokeStyle='#fff'; ctx.lineCap='round'; ctx.stroke();
    ctx.beginPath(); ctx.arc(cx,cy,6,0,2*Math.PI);
    ctx.fillStyle='#fff'; ctx.fill();
    // value text
    const safe=Number.isFinite(+iaq);
    ctx.fillStyle='#fff'; ctx.font='bold 22px Inter,sans-serif'; ctx.textAlign='center';
    ctx.fillText(safe?Math.round(+iaq):'--', cx, cy-14);
    // label
    let lbl='No data';
    if(safe) { const b=bands.find(b=>val>=b.lo&&val<b.hi)||bands[bands.length-1]; lbl=b.lbl; }
    const lcol=safe?(bands.find(b=>val>=b.lo&&val<b.hi)||bands[bands.length-1]).col:'#888';
    ctx.fillStyle=lcol; ctx.font='bold 11px Inter,sans-serif';
    ctx.fillText(lbl, cx, cy+6);
}

// ══════════════════════════════════════════════════════
// WIND ROSE COMPASS  — SVG needle
// ══════════════════════════════════════════════════════
function windRose(dir_deg, speed_kph, gust_kph){
    const safe=Number.isFinite(+dir_deg);
    const ang=safe?+dir_deg:0;
    const dirs=['N','NE','E','SE','S','SW','W','NW'];
    const card=safe?dirs[Math.round(ang/45)%8]:'--';
    const spd=Number.isFinite(+speed_kph)?fv(speed_kph,1):'--';
    const gst=Number.isFinite(+gust_kph)?fv(gust_kph,1):'--';
    // SVG compass
    const marks=dirs.map((d,i)=>{
        const a=(i*45-90)*Math.PI/180;
        const r1=38, r2=43, tx=50+Math.cos(a)*48, ty=50+Math.sin(a)*48;
        return `<line x1="${50+Math.cos(a)*r1}" y1="${50+Math.sin(a)*r1}" x2="${50+Math.cos(a)*r2}" y2="${50+Math.sin(a)*r2}" stroke="rgba(255,255,255,.3)" stroke-width="1.5"/>
        <text x="${tx}" y="${ty}" text-anchor="middle" dominant-baseline="middle" fill="${d==='N'?'#f97316':'rgba(255,255,255,.5)'}" font-size="${d==='N'||d==='S'||d==='E'||d==='W'?8:6}" font-family="Inter,sans-serif" font-weight="${d==='N'?'bold':'normal'}">${d}</text>`;
    }).join('');
    const na=(ang-90)*Math.PI/180;
    const nx=50+Math.sin((ang)*Math.PI/180)*30, ny=50-Math.cos((ang)*Math.PI/180)*30;
    const needle=safe?`<line x1="50" y1="50" x2="${nx}" y2="${ny}" stroke="#ef4444" stroke-width="3" stroke-linecap="round"/>
    <line x1="50" y1="50" x2="${50-Math.sin(ang*Math.PI/180)*12}" y2="${50+Math.cos(ang*Math.PI/180)*12}" stroke="rgba(255,255,255,.4)" stroke-width="2" stroke-linecap="round"/>`:'';
    return `<div class="wind-wrap">
        <svg width="100" height="100" viewBox="0 0 100 100">
            <circle cx="50" cy="50" r="46" fill="rgba(0,0,0,.3)" stroke="rgba(255,255,255,.1)" stroke-width="1"/>
            <circle cx="50" cy="50" r="32" fill="none" stroke="rgba(255,255,255,.05)" stroke-width="1"/>
            ${marks}${needle}
            <circle cx="50" cy="50" r="4" fill="#fff"/>
        </svg>
        <div class="wind-info">
            <div class="wind-kv"><span>Speed</span><span>${spd} kph</span></div>
            <div class="wind-kv"><span>Gust</span><span style="color:#f59e0b">${gst} kph</span></div>
            <div class="wind-kv"><span>Dir</span><span style="color:#60a5fa">${card} ${safe?Math.round(ang)+'°':''}</span></div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// PRESSURE TREND + ZAMBRETTI FORECAST
// ══════════════════════════════════════════════════════
function pressureTrend(press_hpa, delta3h, trend_str){
    const p=Number.isFinite(+press_hpa)?+press_hpa:null;
    const d=Number.isFinite(+delta3h)?+delta3h:null;
    const arrows={Rising:'↑',Falling:'↓','Rapidly Rising':'⇈','Rapidly Falling':'⇊',Stable:'→'};
    let dir='→', col='#94a3b8', zam='Changeable';
    if(d!=null){
        if(d>3){dir='⇈';col='#22c55e';zam='Fair, improving';}
        else if(d>1){dir='↑';col='#4ade80';zam='Clearing, good';}
        else if(d>-1){dir='→';col='#94a3b8';zam='Settled, no change';}
        else if(d>-3){dir='↓';col='#f59e0b';zam='Unsettled, rain possible';}
        else{dir='⇊';col='#ef4444';zam='Deteriorating, rain/storm';}
    } else if(trend_str && arrows[trend_str]){ dir=arrows[trend_str]; }
    const dStr=d!=null?(d>0?'+':'')+fv(d,1)+' hPa/3h':'--';
    return `<div class="press-card">
        <div class="press-arrow" style="color:${col}">${dir}</div>
        <div class="press-info">
            <div class="press-val" style="color:${col}">${p!=null?fv(p,1):'--.--'} <span style="font-size:13px;font-weight:400;color:var(--text-dim)">hPa</span></div>
            <div class="press-forecast">${zam}</div>
            <div class="press-delta" style="color:${col}">${dStr}</div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// FAULT / HEALTH LED MATRIX
// ══════════════════════════════════════════════════════
function faultMatrix(s){
    const sensors=[
        ['SHT45','Fault_SHT45'],['BMP585','Fault_BMP585'],['BME690','Fault_BME690'],
        ['SCD41','Fault_SCD41'],['SGP41','Fault_SGP41'],['BMV080','Fault_BMV080'],
        ['TSL2591','Fault_TSL2591'],['LTR390','Fault_LTR390'],['VEML','Fault_VEML7700'],
        ['OPT4048','Fault_OPT4048'],['TCS','Fault_TCS34725'],['AS7343','Fault_AS7343'],
        ['AS7331','Fault_AS7331'],['AS3935','Fault_AS3935'],
        ['INA219','Fault_INA219'],['MAX17048','Fault_MAX17048'],
        ['LSM6','Fault_LSM6DSOX'],['LIS3MDL','Fault_LIS3MDL'],['BMM350','Fault_BMM350'],
        ['ZMOD4510','Fault_ZMOD4510'],['ILPS','Fault_ILPS22QS'],['MS8607','Fault_MS8607'],
        ['RG15','Fault_RG15'],['I2C Mem','Fault_I2CMemory'],
    ];
    return sensors.map(([name,key])=>{
        const v=s[key];
        let cls='na', tip='N/A';
        if(v!=null){ cls=(String(v)==='true'||String(v)==='1'||String(v)==='ON')?'bad':'ok'; tip=cls==='bad'?'FAULT':'OK'; }
        return `<div class="fault-item" title="${name}: ${tip}"><div class="fault-dot ${cls}"></div><span>${name}</span></div>`;
    }).join('');
}

// ══════════════════════════════════════════════════════
// MINI SPARKLINE INSIDE SENSOR TILE
// ══════════════════════════════════════════════════════
function sensorTileSpark(icon,label,val,unit,status,histKey,color){
    const hist=CHART_HISTORY[histKey]||[];
    const cid='spk_'+histKey+'_'+Math.random().toString(36).slice(2,6);
    // draw after render
    requestAnimationFrame(()=>{
        const cvs=$(cid); if(!cvs||hist.length<2) return;
        const W=cvs.offsetWidth||90, H=28;
        cvs.width=W; cvs.height=H;
        const ctx=cvs.getContext('2d');
        const mn=Math.min(...hist), mx=Math.max(...hist), rng=mx-mn||1;
        const pts=hist.map((v,i)=>[W*i/(hist.length-1), H-H*((v-mn)/rng)*0.9]);
        const grad=ctx.createLinearGradient(0,0,0,H);
        grad.addColorStop(0,color+'66'); grad.addColorStop(1,color+'00');
        ctx.beginPath(); ctx.moveTo(pts[0][0],H);
        pts.forEach(p=>ctx.lineTo(p[0],p[1]));
        ctx.lineTo(pts[pts.length-1][0],H); ctx.closePath();
        ctx.fillStyle=grad; ctx.fill();
        ctx.beginPath(); pts.forEach((p,i)=>i?ctx.lineTo(p[0],p[1]):ctx.moveTo(p[0],p[1]));
        ctx.strokeStyle=color; ctx.lineWidth=1.5; ctx.stroke();
    });
    return `<div class="sensor ${status}" style="padding-bottom:30px">
        <div class="sensor-icon">${icon}</div>
        <div class="sensor-label">${label}</div>
        <div class="sensor-value">${val??'--'}</div>
        <div class="sensor-unit">${unit}</div>
        <canvas class="spark" id="${cid}"></canvas>
    </div>`;
}

// ══════════════════════════════════════════════════════
// ZMOD4510 OUTDOOR GAS PANEL
// ══════════════════════════════════════════════════════
function zmodPanel(s){
    const no2=s.ZMOD4510_NO2_ppb, o3=s.ZMOD4510_O3_ppb,
          eaqi=s.ZMOD4510_EPA_AQI, fast=s.ZMOD4510_FAST_AQI,
          smog=s.ZMOD4510_Smog_Index, o3r=s.ZMOD4510_O3_Risk, st=s.ZMOD4510_Status;
    const no2col=Number.isFinite(+no2)?(+no2>200?'#ef4444':+no2>100?'#f59e0b':'#22c55e'):'#888';
    const o3col=Number.isFinite(+o3)?(+o3>120?'#ef4444':+o3>70?'#f59e0b':'#22c55e'):'#888';
    const aqcol=Number.isFinite(+eaqi)?(+eaqi>150?'#ef4444':+eaqi>100?'#f59e0b':+eaqi>50?'#84cc16':'#22c55e'):'#888';
    return `<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">
        <div style="background:rgba(0,0,0,.22);border-radius:10px;padding:12px;border-left:3px solid ${no2col}">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px">NO₂</div>
            <div style="font-size:26px;font-weight:800;color:${no2col};line-height:1">${Number.isFinite(+no2)?fv(no2,1):'--'}</div>
            <div style="font-size:9px;color:var(--text-dim);margin-top:2px">ppb &nbsp;·&nbsp; ${Number.isFinite(+s.ZMOD4510_NO2_ugm3)?fv(s.ZMOD4510_NO2_ugm3,1)+' µg/m³':'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.22);border-radius:10px;padding:12px;border-left:3px solid ${o3col}">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px">O₃</div>
            <div style="font-size:26px;font-weight:800;color:${o3col};line-height:1">${Number.isFinite(+o3)?fv(o3,1):'--'}</div>
            <div style="font-size:9px;color:var(--text-dim);margin-top:2px">ppb &nbsp;·&nbsp; ${Number.isFinite(+s.ZMOD4510_O3_ugm3)?fv(s.ZMOD4510_O3_ugm3,1)+' µg/m³':'--'} &nbsp;·&nbsp; Risk: ${o3r??'--'}</div>
        </div>
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-top:6px">
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">EPA AQI</div>
            <div style="font-size:20px;font-weight:800;color:${aqcol}">${Number.isFinite(+eaqi)?Math.round(+eaqi):'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">Fast AQI</div>
            <div style="font-size:20px;font-weight:800;color:var(--text)">${Number.isFinite(+fast)?Math.round(+fast):'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">Smog Idx</div>
            <div style="font-size:20px;font-weight:800;color:${Number.isFinite(+smog)&&+smog>50?'#f59e0b':'var(--text)'}">${Number.isFinite(+smog)?fv(smog,0):'--'}</div>
        </div>
    </div>
    <div style="margin-top:6px;font-size:10px;color:var(--text-dim);text-align:right">Sensor: ${st??'--'}</div>`;
}

// ── Big readable metric card — compact data card ──
function bigCard(icon,title,value,unit,note,color='#00d4aa',status=''){
    const col = status==='danger'?'#ef4444':status==='warn'?'#f59e0b':color;
    const bg  = status==='danger'?'rgba(239,68,68,.08)':status==='warn'?'rgba(245,158,11,.08)':'rgba(0,0,0,.18)';
    return `<div style="background:${bg};border-radius:11px;padding:13px 14px;
                        border:1px solid rgba(255,255,255,.07);border-left:3px solid ${col};
                        margin-bottom:7px;display:flex;align-items:center;gap:12px">
        <div style="font-size:22px;line-height:1;flex-shrink:0">${icon}</div>
        <div style="flex:1;min-width:0">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;
                letter-spacing:.6px;font-weight:700;margin-bottom:1px">${title}</div>
            <div style="font-size:22px;font-weight:800;color:${col};line-height:1.1;
                font-family:'JetBrains Mono',monospace">${value}
                <span style="font-size:11px;font-weight:400;color:var(--text-dim)">${unit}</span>
            </div>
            ${note?`<div style="font-size:10px;color:var(--text-dim);margin-top:2px">${note}</div>`:''}
        </div>
        ${status==='danger'?'<div style="font-size:16px">⚠️</div>':status==='warn'?'<div style="font-size:16px">⚡</div>':''}
    </div>`;
}

// ── Risk progress bar — with threshold tick ──
function riskBar(label, val, maxVal=10, color='#00d4aa'){
    const n = typeof val==='number'?val:parseFloat(val)||0;
    const pct = Math.min(100,(n/maxVal)*100);
    const col = pct>70?'#ef4444':pct>40?'#f59e0b':color;
    const fillGrad = pct>70
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 40%,#ef4444)'
        : pct>40 ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)';
    return `<div style="margin-bottom:11px">
        <div style="display:flex;justify-content:space-between;align-items:baseline;
            font-size:11px;margin-bottom:5px">
            <span style="color:var(--text-dim)">${label}</span>
            <span style="color:${col};font-weight:800;font-family:'JetBrains Mono',monospace">
                ${n.toFixed(1)}<span style="font-size:9px;opacity:.6"> /${maxVal}</span>
            </span>
        </div>
        <div style="position:relative;height:8px">
            <div style="width:100%;height:100%;border-radius:999px;
                background:rgba(255,255,255,.07)"></div>
            <div style="position:absolute;left:0;top:0;height:100%;border-radius:999px;
                background:${fillGrad};width:${pct}%;
                transition:width .7s cubic-bezier(.4,0,.2,1);
                box-shadow:0 0 8px ${col}66"></div>
        </div>
    </div>`;
}

// ── Load section ──
function loadSection(id){
    setActiveBtn(id);
    if(id==='dash')     loadDashboard();
    else if(id==='medical')  loadMedical();
    else if(id==='space')    loadSpaceWeather();
    else if(id==='environ')  loadEnviron();
    else if(id==='spectr')   loadSpectrometer();
    
    else if(id==='imu')      loadIMU();
    else if(id==='sensors')  loadSensors();
    else if(id==='charts')   loadCharts();
    else if(id==='sys')      loadSys();
    else if(id==='log')      loadLogs();
}

// ═══════════════════════════════════════════════════════════════
// DASHBOARD  — full featured v3
// ═══════════════════════════════════════════════════════════════
let _dashTimer = null;
function loadDashboard(){
    const vw=window.innerWidth, W=Math.min(vw-16,480);
    createWindow('dash-win','Dashboard','◉',20,40,`
        <!-- ROW 1: Battery donut + AQI dial + Clock -->
        <div class="card-2col" style="margin-bottom:9px">
            <div class="card" style="padding:12px">
                <div class="card-title">🔋 Battery</div>
                <div id="dBatDonut"></div>
            </div>
            <div class="card" style="padding:12px">
                <div class="card-title">🍃 Air Quality Index</div>
                <div class="aqi-wrap">
                    <canvas id="dAqiDial" width="200" height="120" style="max-width:100%"></canvas>
                    <div id="dAqiLabel" class="aqi-label"></div>
                </div>
            </div>
        </div>
        <!-- ROW 2: Safety gauges -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🧭 Safety Gauges</div>
            <div class="hero-grid" id="dGauges"></div>
        </div>
        <!-- ROW 3: Pressure trend + Wind rose + Clock -->
        <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:9px;margin-bottom:9px">
            <div class="card" style="padding:12px">
                <div class="card-title">🌡️ Pressure</div>
                <div id="dPressure"></div>
            </div>
            <div class="card" style="padding:12px">
                <div class="card-title">🌬️ Wind</div>
                <div id="dWind"></div>
            </div>
            <div class="card" style="padding:12px;text-align:center">
                <div class="card-title">🕒 Local Time</div>
                <div id="dClock"></div>
                <div id="dMode" style="margin-top:6px;font-size:10px;color:var(--text-dim)"></div>
            </div>
        </div>
        <!-- ROW 4: Environment tiles with sparklines -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🏠 Environment</div>
            <div class="card-grid" id="dEnv"></div>
        </div>
        <!-- ROW 5: Outdoor gas ZMOD4510 -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🏭 Outdoor Air — ZMOD4510 (NO₂ / O₃)</div>
            <div id="dZmod"></div>
        </div>
        <!-- ROW 6: Radiation & Light -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">⚡ Radiation & Light</div>
            <div class="card-grid" id="dRad"></div>
        </div>
        <!-- ROW 7: Fault matrix -->
        <div class="card">
            <div class="card-title">🔧 Hardware Health</div>
            <div class="fault-grid" id="dFault"></div>
        </div>
    `,W);

    function renderDash(){
        fetch('/api').then(r=>r.json()).then(d=>{
            const s=d.sensors||{};

            // Battery donut
            const dbd=$('dBatDonut');
            if(dbd) dbd.innerHTML=batteryDonut(
                s.BMS_State_Of_Charge, s.BMS_Cell_Voltage,
                s.SOLAR_Power_mW, s.BMS_ESP_Power_mW,
                s.BMS_Drain_Rate||s.BMS_SOC_Rate, s.BMS_Time_To_Empty_min);

            // AQI dial
            drawAqiDial('dAqiDial', s.BME688_IAQ);
            const aqL=$('dAqiLabel');
            if(aqL){
                const v=+s.BME688_IAQ;
                const acc=s.BME688_IAQ_Accuracy!=null?(' acc:'+s.BME688_IAQ_Accuracy):'';
                aqL.textContent='IAQ '+fv(s.BME688_IAQ,0)+acc;
                aqL.style.color=v>200?'#ef4444':v>100?'#f59e0b':'#22c55e';
            }

            // Safety gauges
            const dg=$('dGauges');
            if(dg) dg.innerHTML=[
                gauge('IAQ',s.BME688_IAQ,0,300,100,200,''),
                gauge('CO₂',s.SCD41_CO2_ppm,400,2500,1000,2000,'ppm'),
                gauge('PM2.5',s.BMV080_PM2_5,0,150,35,55,'µg/m³'),
                gauge('UV',s.LTR390_UVI,0,11,3,7,''),
                gauge('Radiation',s.Geiger_uSvh,0,1.0,0.2,0.5,'µSv/h'),
                gauge('Humidity',s.SHT45_Hum,0,100,60,75,'%'),
            ].join('');

            // Pressure trend
            const dp=$('dPressure');
            if(dp) dp.innerHTML=pressureTrend(s.BMP585_Pressure_hPa, s.METEO_Pressure_3h_Delta, s.BMP585_Trend);

            // Wind
            const dw=$('dWind');
            if(dw) dw.innerHTML=windRose(s.WIND_Direction_Deg, s.WIND_Speed_Kph, s.WIND_Gust_Kph);

            // Clock
            const dc=$('dClock');
            if(dc && !dc._inited){ dc.innerHTML=analogClockHTML(); dc._inited=true;
                if(_dashTimer) clearInterval(_dashTimer);
                _dashTimer=setInterval(tickDashClock,1000); tickDashClock(); }
            const dm=$('dMode');
            if(dm) dm.innerHTML=`Mode: <b>${s.System_Mode||'--'}</b> &nbsp;·&nbsp; Up: ${s.System_Uptime_Hours!=null?fv(s.System_Uptime_Hours,1)+'h':'--'}`;

            // Environment tiles with sparklines
            const de=$('dEnv');
            if(de) de.innerHTML=[
                sensorTileSpark('🌡️','Temperature',fv(s.SHT45_Temp),'°C',s.SHT45_Temp>35?'danger':s.SHT45_Temp>28?'warn':'good','SHT45_Temp','#f97316'),
                sensorTileSpark('🌡️','Feels Like',fv(s.METEO_Feels_Like_C),'°C','good','METEO_Feels_Like_C','#fb923c'),
                sensorTileSpark('💧','Humidity',fv(s.SHT45_Hum),'%',s.SHT45_Hum>75?'warn':'good','SHT45_Hum','#38bdf8'),
                sensorTileSpark('📊','Pressure',fv(s.BMP585_Pressure_hPa,0),'hPa','good','BMP585_Pressure_hPa','#94a3b8'),
                sensorTileSpark('🫧','CO₂',fv(s.SCD41_CO2_ppm,0),'ppm',status_co2(s.SCD41_CO2_ppm),'SCD41_CO2_ppm','#a3e635'),
                sensorTileSpark('💨','VOC Idx',fv(s.SGP41_VOC_Index,0),'',s.SGP41_VOC_Index>150?'warn':'good','SGP41_VOC_Index','#c084fc'),
            ].join('');

            // ZMOD4510
            const dz=$('dZmod');
            if(dz) dz.innerHTML=zmodPanel(s);

            // Radiation & Light
            const dr=$('dRad');
            if(dr) dr.innerHTML=[
                sensorTileSpark('☢️','Radiation',fv(s.Geiger_uSvh,3),'µSv/h',status_rad(s.Geiger_uSvh),'Geiger_uSvh','#facc15'),
                
                sensorTileSpark('🌞','UV Index',fv(s.LTR390_UVI,1),'',s.LTR390_UVI>7?'danger':s.LTR390_UVI>3?'warn':'good','LTR390_UVI','#e879f9'),
                sensorTile('🔆','Lux',fv(s.TSL2591_Lux,0),'lx','good'),
                sensorTile('🌈','CCT',fv(s.OPTICS_CCT,0),'K','good'),
                sensorTile('🧲','Mag',fv(s.LIS3MDL_Mag_uT),'µT','good'),
            ].join('');

            // Fault matrix
            const df=$('dFault');
            if(df) df.innerHTML=faultMatrix(s);

        }).catch(()=>{});
    }

    renderDash();
    // auto-refresh data every 30 s (clock ticks every 1s via _dashTimer)
    if(window._dashDataTimer) clearInterval(window._dashDataTimer);
    window._dashDataTimer=setInterval(renderDash, 30000);
}

// ═══════════════════════════════════════════════
// ALL SENSORS — 2-column list with copy-on-click
// ═══════════════════════════════════════════════
function loadSensors(){
    const vw=window.innerWidth;
    const w=Math.min(vw-16, 720);
    createWindow('sensors-win','All Sensors','📡',20,40,`
        <input class="log-search" placeholder="🔍 Filter sensors..." id="sFilter" oninput="filterSensors()">
        <div id="sList" style="margin-top:8px"></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        window._sensorData=s;
        renderSensorList(s,'');
    });
}
function renderSensorList(s,q){
    const keys=Object.keys(s).sort().filter(k=>!q||k.toLowerCase().includes(q.toLowerCase())||String(s[k]).toLowerCase().includes(q.toLowerCase()));
    const half=Math.ceil(keys.length/2);
    const col=(arr)=>arr.map(k=>{
        let v=s[k];
        if(v==null) v='null';
        else if(typeof v==='number') v=v.toFixed(v>1000?0:v>100?1:2);
        return `<div class="srow" onclick="copySensor('${k}','${v}')">
            <span>${k}</span><span>${v}</span>
        </div>`;
    }).join('');
    $('sList').innerHTML=`<div style="display:grid;grid-template-columns:1fr 1fr;gap:0 16px">
        <div>${col(keys.slice(0,half))}</div>
        <div>${col(keys.slice(half))}</div>
    </div><div style="font-size:10px;color:var(--text-muted);margin-top:6px;text-align:right">${keys.length} sensors — click to copy</div>`;
}
function filterSensors(){ const q=($('sFilter')||{}).value||''; if(window._sensorData) renderSensorList(window._sensorData,q); }
function copySensor(k,v){
    navigator.clipboard?.writeText(k+': '+v).then(()=>showToast('Copied: '+k));
}

// ═══════════════════════════════════════════════
// ENVIRONMENT
// ═══════════════════════════════════════════════
function loadEnv(){
    createWindow('env-win','Environment','🌬️',60,60,`
        <div class="card">
            <div class="card-title">🌡️ Thermal Comfort</div>
            <div class="card-grid" id="eThermal"></div>
        </div>
        <div class="card-2col">
            <div class="card"><div class="card-title">💧 Humidity</div><div id="eHum"></div></div>
            <div class="card"><div class="card-title">🌬️ Pressure</div><div id="ePress"></div></div>
        </div>
        <div class="card"><div class="card-title">☀️ Radiation</div><div class="card-grid" id="eRad"></div></div>
        <div class="card"><div class="card-title">⚡ Power</div><div class="card-grid" id="ePwr"></div></div>
    `,390);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        $('eThermal').innerHTML=[
            sensorTile('🌡️','Actual',fv(s.SHT45_Temp),'°C','good'),
            sensorTile('🌡️','Feels Like',fv(s.METEO_Feels_Like_C),'°C','good'),
            sensorTile('🌡️','Heat Index',fv(s.METEO_Heat_Index),'°C','good'),
            sensorTile('💧','Wet Bulb',fv(s.METEO_Wet_Bulb_C),'°C','good'),
            sensorTile('⛅','Dew Point',fv(s.METEO_Dew_Point_C),'°C','good'),
            sensorTile('🌬️','Wind Chill',fv(s.WIND_Speed_Kph)?fv(s.METEO_Feels_Like_C):'--','°C','good'),
        ].join('');
        $('eHum').innerHTML=[
            sensorTile('💧','Rel. Hum.',fv(s.SHT45_Hum),'%',s.SHT45_Hum>75?'warn':'good'),
            sensorTile('💧','Abs. Hum.',fv(s.METEO_Abs_Hum_g_m3),'g/m³','good'),
            sensorTile('🦠','Mold Risk',s.METEO_Mold_Risk?'YES':'No','',s.METEO_Mold_Risk?'warn':'good'),
            sensorTile('🦠','Virus Risk',s.METEO_Virus_Risk?'HIGH':'Low','',s.METEO_Virus_Risk?'warn':'good'),
        ].join('');
        $('ePress').innerHTML=[
            sensorTile('📊','Station',fv(s.BMP585_Pressure_hPa,1),'hPa','good'),
            sensorTile('📊','Sea Level',fv(s.METEO_Sea_Level_Press_hPa,1),'hPa','good'),
            sensorTile('☁️','Cloud Base',fv(s.METEO_Cloud_Base_m,0),'m','good'),
            sensorTile('💨','Air Density',fv(s.METEO_Air_Density,4),'kg/m³','good'),
        ].join('');
        $('eRad').innerHTML=[
            sensorTile('☢️','uSv/h',fv(s.Geiger_uSvh,3),'µSv/h',status_rad(s.Geiger_uSvh)),
            
            sensorTile('🌞','UV Index',fv(s.LTR390_UVI),'',s.LTR390_UVI>7?'danger':s.LTR390_UVI>3?'warn':'good'),
            sensorTile('🔆','Lux',fv(s.TSL2591_Lux,0),'lx','good'),
        ].join('');
        $('ePwr').innerHTML=[
            sensorTile('🔋','Battery',fv(s.BMS_State_Of_Charge,0),'%',s.BMS_State_Of_Charge<20?'warn':'good'),
            sensorTile('☀️','Solar',fv(s.SOLAR_Power_mW,0),'mW','good'),
            sensorTile('⚡','Node',fv(s.BMS_ESP_Power_mW,0),'mW','good'),
            sensorTile('⏱️','T to Empty',fv(s.BMS_Time_To_Empty_min,0),'min','good'),
        ].join('');
    });
}

// ═══════════════════════════════════════════════
// IMU + MAGNETOMETERS
// ═══════════════════════════════════════════════
function loadIMU(){
    createWindow('imu-win','IMU & Magnetometers','🔭',80,60,`
        <div class="card-2col">
            <div class="card">
                <div class="card-title">🔩 LSM6DSOX Accelerometer</div>
                <div id="iAccel"></div>
            </div>
            <div class="card">
                <div class="card-title">🔄 LSM6DSOX Gyroscope</div>
                <div id="iGyro"></div>
            </div>
        </div>
        <div class="card-2col">
            <div class="card">
                <div class="card-title">🧲 LIS3MDL Magnetometer</div>
                <div id="iLis"></div>
            </div>
            <div class="card">
                <div class="card-title">🧲 BMM350 Precision Mag</div>
                <div id="iBmm"></div>
            </div>
        </div>
        <div class="card">
            <div class="card-title">📐 Derived Orientation</div>
            <div class="card-grid" id="iDerived"></div>
        </div>
    `,440);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        $('iAccel').innerHTML=[
            sensorTile('➡️','X',fv(s.LSM6_Accel_X,3),'m/s²','good'),
            sensorTile('⬆️','Y',fv(s.LSM6_Accel_Y,3),'m/s²','good'),
            sensorTile('⬇️','Z',fv(s.LSM6_Accel_Z,3),'m/s²','good'),
            sensorTile('📏','|A|',fv(s.LSM6_Accel_Mag,3),'m/s²',s.LSM6_Shock_Det?'danger':'good'),
        ].join('');
        $('iGyro').innerHTML=[
            sensorTile('🔄','Gx',fv(s.LSM6_Gyro_X_rads,4),'rad/s','good'),
            sensorTile('🔄','Gy',fv(s.LSM6_Gyro_Y_rads,4),'rad/s','good'),
            sensorTile('🔄','Gz',fv(s.LSM6_Gyro_Z_rads,4),'rad/s','good'),
            sensorTile('🌡️','Die T',fv(s.LSM6_Temp_C),'°C','good'),
        ].join('');
        $('iLis').innerHTML=[
            sensorTile('🧲','X',fv(s.LIS3MDL_X_uT),'µT','good'),
            sensorTile('🧲','Y',fv(s.LIS3MDL_Y_uT),'µT','good'),
            sensorTile('🧲','Z',fv(s.LIS3MDL_Z_uT),'µT','good'),
            sensorTile('🧭','Hdg',fv(s.LIS3MDL_Heading,1),'°','good'),
        ].join('');
        $('iBmm').innerHTML=[
            sensorTile('🧲','X',fv(s.BMM350_Mag_X),'µT','good'),
            sensorTile('🧲','Y',fv(s.BMM350_Mag_Y),'µT','good'),
            sensorTile('🧲','Z',fv(s.BMM350_Mag_Z),'µT','good'),
            sensorTile('🧭','Hdg',fv(s.BMM350_Heading_Deg,1),'°','good'),
        ].join('');
        $('iDerived').innerHTML=[
            sensorTile('📐','Tilt',fv(s.LSM6_Tilt_Deg,1),'°','good'),
            sensorTile('〰️','Vibration',fv(s.LSM6_Vibration,3),'m/s²',s.LSM6_Vibration>0.5?'warn':'good'),
            sensorTile('⚡','Shock',s.LSM6_Shock_Det?'YES':'No','',s.LSM6_Shock_Det?'danger':'good'),
            sensorTile('🧭','Cardinal',s.BMM350_Cardinal||'--','','good'),
        ].join('');
    });
}

// ═══════════════════════════════════════════════
// SESSION CHARTS — 10-min rolling sparklines
// ═══════════════════════════════════════════════
const CHART_HISTORY = {};
const CHART_MAX = 60; // 60 samples @ ~10s = 10 min
const CHART_DEFS = [
    {key:'SHT45_Temp',      label:'Temperature',   unit:'°C',   color:'#f97316'},
    {key:'METEO_Feels_Like_C',label:'Feels Like',  unit:'°C',   color:'#fb923c'},
    {key:'SHT45_Hum',       label:'Humidity',      unit:'%',    color:'#38bdf8'},
    {key:'SCD41_CO2_ppm',   label:'CO₂',           unit:'ppm',  color:'#a3e635'},
    {key:'BME688_IAQ',      label:'IAQ',           unit:'',     color:'#34d399'},
    {key:'BMV080_PM2_5',    label:'PM2.5',         unit:'µg/m³',color:'#f43f5e'},
    {key:'SGP41_VOC_Index', label:'VOC Index',     unit:'',     color:'#c084fc'},
    {key:'BMP585_Pressure_hPa',label:'Pressure',   unit:'hPa',  color:'#94a3b8'},
    
    {key:'BMS_State_Of_Charge',label:'Battery',    unit:'%',    color:'#4ade80'},
    {key:'SOLAR_Power_mW',  label:'Solar Power',   unit:'mW',   color:'#fde68a'},
    {key:'LTR390_UVI',      label:'UV Index',      unit:'',     color:'#e879f9'},
];

function loadCharts(){
    const vw=window.innerWidth;
    const w=Math.min(vw-16,800);
    const cards=CHART_DEFS.map(c=>`
        <div class="card">
            <div class="chartbox" style="--chart-h:72px">
                <span class="chart-label">${c.label}</span>
                <canvas id="chart_${c.key}"></canvas>
                <span class="chart-cur" id="cur_${c.key}">--${c.unit}</span>
            </div>
        </div>
    `).join('');
    createWindow('charts-win','Session Charts','📈',100,50,
        `<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">${cards}</div>
         <div style="font-size:10px;color:var(--text-muted);margin-top:6px;text-align:center">Rolling 10-min session history (current session only)</div>`,
        w);
    pollCharts();
}

function drawSparkline(canvasId, data, color){
    const cvs=$(canvasId); if(!cvs) return;
    const W=cvs.clientWidth||cvs.offsetWidth||200;
    const H=cvs.clientHeight||cvs.offsetHeight||72;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    ctx.clearRect(0,0,W,H);
    if(data.length<2) return;
    const mn=Math.min(...data), mx=Math.max(...data);
    const range=mx-mn||1;
    const pad=4;
    const pts=data.map((v,i)=>[
        pad+(W-2*pad)*(i/(data.length-1)),
        H-pad-(H-2*pad)*((v-mn)/range)
    ]);
    // fill
    const grad=ctx.createLinearGradient(0,0,0,H);
    grad.addColorStop(0,color+'55'); grad.addColorStop(1,color+'00');
    ctx.beginPath(); ctx.moveTo(pts[0][0],H);
    pts.forEach(p=>ctx.lineTo(p[0],p[1]));
    ctx.lineTo(pts[pts.length-1][0],H); ctx.closePath();
    ctx.fillStyle=grad; ctx.fill();
    // line
    ctx.beginPath(); ctx.moveTo(pts[0][0],pts[0][1]);
    pts.forEach(p=>ctx.lineTo(p[0],p[1]));
    ctx.strokeStyle=color; ctx.lineWidth=1.5; ctx.stroke();
    // dot
    const lp=pts[pts.length-1];
    ctx.beginPath(); ctx.arc(lp[0],lp[1],3,0,Math.PI*2);
    ctx.fillStyle=color; ctx.fill();
}

function pollCharts(){
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        CHART_DEFS.forEach(c=>{
            const v=s[c.key];
            if(v==null) return;
            if(!CHART_HISTORY[c.key]) CHART_HISTORY[c.key]=[];
            CHART_HISTORY[c.key].push(+v);
            if(CHART_HISTORY[c.key].length>CHART_MAX) CHART_HISTORY[c.key].shift();
            const cur=$('cur_'+c.key);
            if(cur) cur.textContent=fv(v)+(c.unit?' '+c.unit:'');
            drawSparkline('chart_'+c.key, CHART_HISTORY[c.key], c.color);
        });
    }).catch(()=>{});
    setTimeout(pollCharts,10000);
}

// ═══════════════════════════════════════════════
// AUTH — password modal for System panel
// ═══════════════════════════════════════════════
const SYS_PASS = 'atlas2025';   // change as needed
let sysUnlocked = false;

function loadSys(){
    if(!sysUnlocked){
        showPassModal(()=>{ sysUnlocked=true; loadSys(); });
        return;
    }
    _buildSysWindow();
}

function showPassModal(onSuccess){
    // Remove any existing modal
    const old=$('pass-modal'); if(old) old.remove();

    const modal = document.createElement('div');
    modal.id='pass-modal';
    modal.style.cssText=`
        position:fixed;inset:0;z-index:20000;
        background:rgba(0,0,0,.65);backdrop-filter:blur(10px);
        display:flex;align-items:center;justify-content:center`;
    modal.innerHTML=`
        <div style="background:rgba(28,28,44,.96);border:1px solid rgba(255,255,255,.12);
                    border-radius:18px;padding:32px 28px;width:320px;max-width:92vw;
                    box-shadow:0 20px 60px rgba(0,0,0,.5);animation:winIn .3s ease">
            <div style="font-size:24px;text-align:center;margin-bottom:6px">🔒</div>
            <div style="font-size:14px;font-weight:600;color:var(--text);text-align:center;margin-bottom:4px">System Access</div>
            <div style="font-size:11px;color:var(--text-muted);text-align:center;margin-bottom:20px">Enter the system password to continue</div>
            <input id="pass-inp" type="password" placeholder="Password…"
                   style="text-align:center;font-size:14px;letter-spacing:.2em;margin-bottom:12px"
                   onkeydown="if(event.key==='Enter')$('pass-ok').click()">
            <div id="pass-err" style="color:var(--danger);font-size:11px;text-align:center;height:16px;margin-bottom:8px"></div>
            <div style="display:flex;gap:8px">
                <button class="btn" style="flex:1" onclick="$('pass-modal').remove()">Cancel</button>
                 <button id="pass-ok" class="btn accent" style="flex:1" onclick="checkPass(SYS_PASS)">Unlock</button>
            </div>
        </div>`;
    document.body.appendChild(modal);
    setTimeout(()=>{ const i=$('pass-inp'); if(i) i.focus(); }, 80);
    // store callback
    window._passCallback = onSuccess;
}

function checkPass(expected){
    const val=($('pass-inp')||{}).value||'';
    if(val===expected){
        $('pass-modal').remove();
        if(window._passCallback) window._passCallback();
    } else {
        const e=$('pass-err');
        if(e){ e.textContent='Incorrect password'; }
        const inp=$('pass-inp');
        if(inp){ inp.value=''; inp.style.borderColor='var(--danger)';
            setTimeout(()=>{ if(inp) inp.style.borderColor=''; },1000); }
    }
}

function lockSys(){
    sysUnlocked=false;
    closeWin('sys-win');
    showToast('System locked');
}

const MODE_BTN_MAP = {
    'Continuous':   {action:'mode_cont',  label:'◉ Continuous',  id:'mbtn_cont'},
    'Light_Sleep':  {action:'mode_light', label:'🌙 Light Sleep', id:'mbtn_light'},
    'Deep_Sleep':   {action:'mode_deep',  label:'💤 Deep Sleep',  id:'mbtn_deep'},
    'Maintenance':  {action:'mode_maint', label:'🔧 Maintenance', id:'mbtn_maint'},
};
function setModeBtn(modeStr){
    Object.values(MODE_BTN_MAP).forEach(m=>{
        const b=$(m.id); if(!b) return;
        b.classList.toggle('mode-active', modeStr===m.action.replace('mode_','').replace('_','_'));
    });
    // normalise: "Continuous" matches 'mode_cont'
    const key=Object.keys(MODE_BTN_MAP).find(k=>modeStr&&modeStr.toLowerCase().replace(' ','_')===k.toLowerCase().replace(' ','_'));
    if(key){ const m=MODE_BTN_MAP[key]; const b=$(m.id); if(b) b.classList.add('mode-active'); }
}
function cmdMode(action){
    cmd(action);
    // Optimistically highlight immediately, then re-confirm from API
    const nm=action.replace('mode_','').replace('cont','Continuous').replace('light','Light_Sleep').replace('deep','Deep_Sleep').replace('maint','Maintenance');
    Object.values(MODE_BTN_MAP).forEach(m=>{ const b=$(m.id); if(b) b.classList.remove('mode-active'); });
    const target=Object.values(MODE_BTN_MAP).find(m=>m.action===action);
    if(target){ const b=$(target.id); if(b) b.classList.add('mode-active'); }
}

function _buildSysWindow(){
    createWindow('sys-win','System (Unlocked)','⚙️',60,60,`
        <div class="card"><div class="card-title">🩺 System Health</div><div id="sHealth">Checking…</div></div>
        <div class="card"><div class="card-title">🎯 Operating Mode</div>
            <div id="sModeNow" style="font-size:11px;color:var(--text-dim);margin-bottom:7px">Current: <b id="sModeName">loading…</b></div>
            <div class="btn-group">
                <button id="mbtn_cont"  class="btn mode-btn" onclick="cmdMode('mode_cont')">◉ Continuous</button>
                <button id="mbtn_light" class="btn mode-btn" onclick="cmdMode('mode_light')">🌙 Light Sleep</button>
                <button id="mbtn_deep"  class="btn mode-btn" onclick="cmdMode('mode_deep')">💤 Deep Sleep</button>
                <button id="mbtn_maint" class="btn mode-btn" onclick="cmdMode('mode_maint')">🔧 Maintenance</button>
            </div>
        </div>
        <div class="card"><div class="card-title">🔧 Hardware Controls</div>
            <div class="btn-group">
                <button class="btn" onclick="openI2CScan()">📡 I2C Scanner</button>
                <button class="btn" onclick="cmd('read_all')">📥 Read Sensors</button>
                <button class="btn" onclick="cmd('init_sensors')">🔄 Re-Init HW</button>
                <button class="btn" onclick="cmd('stop_sensors')">💤 Sleep Sensors</button>
            </div>
        </div>
        <div class="card"><div class="card-title">⚠️ Danger Zone</div>
            <div class="btn-group">
                <button class="btn danger" onclick="cmd('reset_i2c')">⚡ Reset I2C</button>
                <button class="btn danger" onclick="cmd('reboot')">🔁 Reboot</button>
                <button class="btn danger" onclick="cmd('factory_reset')">🗑 Factory Reset</button>
                <button class="btn" onclick="lockSys()" style="margin-left:auto">🔒 Lock</button>
            </div>
        </div>
        <div class="card"><div class="card-title">📦 OTA Firmware Update</div>
            <input type="file" id="otafile" accept=".bin" style="margin-bottom:7px">
            <button class="btn accent" onclick="uploadOTA()" style="width:100%">⬆️ Flash Firmware</button>
            <div id="ota-prog"></div>
        </div>
    `,360);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        // Health
        let faults=[],ok=[];
        Object.keys(s).filter(k=>k.startsWith('Fault_')).forEach(k=>{
            const name=k.replace('Fault_','');
            if(s[k]==='ON'||s[k]===true||s[k]==='true') faults.push(`<span style="color:var(--danger);font-size:11px;margin:2px">⚠ ${name}</span>`);
            else ok.push(`<span style="color:var(--accent);font-size:10px;opacity:.6;margin:2px">✓ ${name}</span>`);
        });
        const h=$('sHealth'); if(h) h.innerHTML=(faults.length?faults.join(' '):'<span style="color:var(--accent)">✓ All systems nominal</span>')+'<div style="margin-top:6px">'+ok.join(' ')+'</div>';
        // Mode buttons
        const modeName = s.System_Mode || d.system_mode || '';
        const mn=$('sModeName'); if(mn) mn.textContent=modeName||'unknown';
        // highlight correct button
        const modeKey = modeName.toLowerCase().replace(' ','_');
        Object.keys(MODE_BTN_MAP).forEach(k=>{
            const m=MODE_BTN_MAP[k]; const b=$(m.id); if(!b) return;
            const match=k.toLowerCase().replace(' ','_')===modeKey||
                        m.action.replace('mode_','')===modeKey.replace('_sleep','_sleep');
            b.classList.toggle('mode-active',match);
        });
    });
}

// ═══════════════════════════════════════════════════════════════════
// I2C SCANNER v2 — tooltips, fault dots, EEPROM toggle, discovery
// ═══════════════════════════════════════════════════════════════════

// Full I2C address map (Adafruit + community 2025 list + project specifics)
const I2C_DB = {
    '0x03':{name:'AS3935',desc:'Lightning / EM detector',fault:'Fault_AS3935'},
    '0x0B':{name:'BQ27441',desc:'Fuel gauge (alt address)'},
    '0x10':{name:'VEML7700',desc:'Ambient light sensor',fault:'Fault_VEML7700'},
    '0x14':{name:'BMM350',desc:'Precision 3-axis magnetometer',fault:'Fault_BMM350'},
    '0x18':{name:'LIS2DH',desc:'3-axis accelerometer (alt)'},
    '0x19':{name:'LIS2DH',desc:'3-axis accelerometer'},
    '0x1C':{name:'MMA8452',desc:'3-axis accelerometer'},
    '0x1E':{name:'LIS3MDL / HMC5883',desc:'3-axis magnetometer',fault:'Fault_LIS3MDL'},
    '0x20':{name:'MCP23017',desc:'I/O expander'},
    '0x23':{name:'BH1750',desc:'Digital light sensor'},
    '0x26':{name:'SHT41',desc:'Humidity/Temp (alt)'},
    '0x27':{name:'LCD / SHT21',desc:'LCD backpack or humidity'},
    '0x28':{name:'BNO055',desc:'9-DOF IMU (alt)'},
    '0x29':{name:'TSL2591 / VL53L0X',desc:'Light OR ToF distance',fault:'Fault_TSL2591'},
    '0x2A':{name:'TCS34725 alt',desc:'Color sensor alt addr'},
    '0x29':{name:'TCS34725 / TSL2591',desc:'Color/Light sensor',fault:'Fault_TCS34725'},
    '0x33':{name:'ZMOD4510',desc:'NO2/O3 outdoor air quality',fault:'Fault_ZMOD4510'},
    '0x36':{name:'MAX17048',desc:'LiPo fuel gauge',fault:'Fault_MAX17048'},
    '0x38':{name:'AHT20 / SHT31',desc:'Temperature/humidity sensor'},
    '0x39':{name:'AS7343 / TSL2561',desc:'14-ch spectral / light',fault:'Fault_AS7343'},
    '0x3C':{name:'SSD1306 / SH1106',desc:'0.96" OLED display (128x64)'},
    '0x3D':{name:'SSD1306',desc:'OLED display (alt addr)'},
    '0x40':{name:'INA219 / HTU21D',desc:'Power monitor / humidity',fault:'Fault_INA219'},
    '0x41':{name:'INA219',desc:'Power monitor (alt)'},
    '0x44':{name:'SHT45 / OPT4048',desc:'Humidity+Temp / Color',fault:'Fault_SHT45'},
    '0x45':{name:'SHT30',desc:'Humidity+Temp sensor'},
    '0x47':{name:'BMP585',desc:'Barometric pressure sensor',fault:'Fault_BMP585'},
    '0x48':{name:'ADS1115 / TMP102',desc:'16-bit ADC or temperature'},
    '0x49':{name:'ADS1115',desc:'16-bit ADC (alt)'},
    '0x4A':{name:'ADS1115',desc:'16-bit ADC (alt2)'},
    '0x4B':{name:'ADS1115',desc:'16-bit ADC (alt3)'},
    '0x50':{name:'24Cxx EEPROM',desc:'External EEPROM (BSEC state)',fault:'Fault_I2CMemory'},
    '0x53':{name:'LTR390 / ADXL345',desc:'UV sensor / accelerometer',fault:'Fault_LTR390'},
    '0x57':{name:'BMV080',desc:'Particulate matter PM1/2.5/10',fault:'Fault_BMV080'},
    '0x59':{name:'SGP41',desc:'VOC+NOx gas index sensor',fault:'Fault_SGP41'},
    '0x5A':{name:'CCS811 / MLX90614',desc:'eCO2 VOC / IR thermometer'},
    '0x5B':{name:'CCS811',desc:'eCO2 VOC (alt)'},
    '0x5C':{name:'ILPS22QS / BMP180',desc:'HP pressure+QVAR / pressure',fault:'Fault_ILPS22QS'},
    '0x5D':{name:'ILPS22QS alt',desc:'HP barometric pressure alt'},
    '0x62':{name:'SCD41 / SCD30',desc:'NDIR CO2 sensor',fault:'Fault_SCD41'},
    '0x68':{name:'DS3231 / MPU6050',desc:'RTC or 6-DOF IMU'},
    '0x69':{name:'MPU6050 / ICM-20689',desc:'6-DOF IMU (alt addr)'},
    '0x6A':{name:'LSM6DSOX / L3GD20',desc:'6-DOF IMU (accel+gyro)',fault:'Fault_LSM6DSOX'},
    '0x6B':{name:'LSM6DSOX alt',desc:'6-DOF IMU alt addr'},
    '0x70':{name:'TCA9548A MUX#0',desc:'8-ch I2C multiplexer'},
    '0x71':{name:'TCA9548A MUX#1',desc:'8-ch I2C multiplexer'},
    '0x72':{name:'TCA9548A MUX#2',desc:'8-ch I2C multiplexer'},
    '0x74':{name:'AS7331',desc:'UV-A/B/C spectral sensor',fault:'Fault_AS7331'},
    '0x76':{name:'BME280 / BME680 / MS8607',desc:'Env sensor (addr=0x76)',fault:'Fault_BME280'},
    '0x77':{name:'BME690 / BMP280',desc:'BSEC AI gas sensor',fault:'Fault_BME690'},
    '0x7C':{name:'MCP9808',desc:'Precision temperature sensor'},
};

// Sensors expected on each project MUX/CH for discovery cross-check
const EXPECTED_MAP = [
    {mux:'0x70',ch:0,addr:'0x77',sensor:'BME690'},
    {mux:'0x70',ch:1,addr:'0x6A',sensor:'LSM6DSOX'},
    {mux:'0x70',ch:1,addr:'0x1E',sensor:'LIS3MDL'},
    {mux:'0x70',ch:2,addr:'0x14',sensor:'BMM350'},
    {mux:'0x70',ch:3,addr:'0x50',sensor:'EEPROM'},
    {mux:'0x70',ch:4,addr:'0x36',sensor:'MAX17048'},
    {mux:'0x70',ch:4,addr:'0x40',sensor:'INA219'},
    {mux:'0x70',ch:5,addr:'0x44',sensor:'SHT45'},
    {mux:'0x70',ch:5,addr:'0x47',sensor:'BMP585'},
    {mux:'0x70',ch:6,addr:'0x29',sensor:'TSL2591'},
    {mux:'0x70',ch:6,addr:'0x44',sensor:'OPT4048'},
    {mux:'0x71',ch:0,addr:'0x57',sensor:'BMV080'},
    {mux:'0x71',ch:6,addr:'0x5C',sensor:'ILPS22QS'},
    {mux:'0x72',ch:1,addr:'0x33',sensor:'ZMOD4510'},
    {mux:'0x72',ch:4,addr:'0x62',sensor:'SCD41'},
];

let _i2cFaultData = {};

function openI2CScan(){
    const vw=window.innerWidth, w=Math.min(vw-16,860);
    if($('i2c-win')){ focusWin('i2c-win'); return; }
    createWindow('i2c-win','I2C Bus Scanner','📡',20,40,`
        <div style="display:flex;align-items:center;gap:7px;flex-wrap:wrap;margin-bottom:10px">
            <button class="btn accent" id="scan-btn" onclick="runI2CScan()" style="flex:1;min-width:120px">▶ Scan</button>
            <button class="btn" id="disc-btn" onclick="runDiscovery()" title="Try all known sensor addresses on every MUX channel">🔍 Discovery</button>
            <label style="display:flex;align-items:center;gap:5px;font-size:11px;color:var(--text-dim);cursor:pointer;user-select:none">
                <input type="checkbox" id="excl50" checked style="width:13px;height:13px;accent-color:var(--accent)"> Exclude 0x50 (EEPROM)
            </label>
            <button class="btn" onclick="copyI2CScan()">📋 JSON</button>
            <span id="scan-status" style="font-size:10px;color:var(--text-dim);white-space:nowrap"></span>
        </div>
        <!-- Legend -->
        <div style="display:flex;gap:12px;flex-wrap:wrap;margin-bottom:10px;font-size:10px;color:var(--text-dim)">
            <span><span class="i2c-dot ok" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>OK & assigned</span>
            <span><span class="i2c-dot bad" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>Fault / error</span>
            <span><span class="i2c-dot unknown" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>Present, unassigned</span>
            <span><span class="i2c-dot unassigned" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>In DB, not seen</span>
            <span>❓ hover for info</span>
        </div>
        <div id="i2c-result" style="font-size:11px;min-height:200px"></div>
        <div id="disc-result" style="font-size:11px;margin-top:10px"></div>
    `,w);
    // Load fault data then scan
    fetch('/api').then(r=>r.json()).then(d=>{ _i2cFaultData=d.sensors||{}; }).catch(()=>{});
    runI2CScan();
}

function i2cAddrHTML(a, muxCol){
    const excl = ($('excl50')||{}).checked!==false;
    if(excl && a==='0x50') return '';
    const db = I2C_DB[a];
    const fk = db && db.fault;
    const fv = fk ? _i2cFaultData[fk] : undefined;
    let dotCls='unknown', statusText='Present, not mapped', statusCol='#f59e0b';
    if(db){
        if(fk !== undefined){
            const isBad = (String(fv)==='true'||String(fv)==='1'||String(fv)==='ON');
            dotCls=isBad?'bad':'ok';
            statusText=isBad?'FAULT DETECTED':'Sensor OK';
            statusCol=isBad?'#ef4444':'#22c55e';
        } else {
            dotCls='ok'; statusText='Recognised, no fault key'; statusCol='#94a3b8';
        }
    }
    const name=db?db.name:'Unknown device';
    const desc=db?db.desc:'Not in database';
    return `<span class="i2c-addr">
        <span class="i2c-dot ${dotCls}"></span>
        <span style="color:${muxCol};font-family:'JetBrains Mono',monospace">${a}</span>
        ${db?`<span style="color:var(--text-dim);font-size:9px;max-width:70px;overflow:hidden;text-overflow:ellipsis"> ${db.name}</span>`:''}
        <span class="i2c-qmark">?
            <div class="i2c-tt">
                <div class="i2c-tt-name">${name}</div>
                <div class="i2c-tt-addr">${a}</div>
                <div class="i2c-tt-status" style="color:${statusCol}">${statusText}</div>
                <div class="i2c-tt-desc">${desc}</div>
            </div>
        </span>
    </span>`;
}

function runI2CScan(){
    const btn=$('scan-btn'), st=$('scan-status'), res=$('i2c-result');
    if(!res) return;
    if(btn){ btn.disabled=true; btn.textContent='⏳ Scanning…'; }
    if(st) st.textContent='';
    res.innerHTML=`<div style="color:var(--text-dim);padding:12px 0">⏳ Scanning all MUX channels…</div>`;
    fetch('/api').then(r=>r.json()).then(d=>{ _i2cFaultData=d.sensors||{}; }).catch(()=>{});
    const t0=Date.now();
    fetch('/scan_json').then(r=>r.json()).then(data=>{
        if(st) st.textContent=`Done in ${Date.now()-t0}ms`;
        if(btn){ btn.disabled=false; btn.textContent='▶ Scan'; }
        renderI2CScan(data);
    }).catch(err=>{
        if(btn){ btn.disabled=false; btn.textContent='▶ Scan'; }
        if(res) res.innerHTML=`<div style="color:var(--danger)">✗ ${err}</div>`;
    });
}

function renderI2CScan(data){
    const res=$('i2c-result'); if(!res) return;
    const mux_colors=['#00d4aa','#a78bfa','#f59e0b'];
    let html='';

    if(data.muxes) data.muxes.forEach((mux,mi)=>{
        const col=mux_colors[mi]||'#888';
        html+=`<div style="margin-bottom:14px">
            <div style="font-weight:700;color:${col};margin-bottom:7px;font-size:12px;
                border-bottom:1px solid rgba(255,255,255,.07);padding-bottom:4px">
                ■ MUX ${mux.addr}</div>`;
        if(mux.channels) mux.channels.forEach(ch=>{
            const devs=ch.devices||[], offline=ch.status==='OFFLINE';
            html+=`<div style="display:flex;align-items:center;gap:6px;padding:4px 0;
                              border-bottom:1px solid rgba(255,255,255,.04)">
                <span style="color:${offline?'var(--danger)':'var(--text-muted)'};
                    min-width:46px;font-size:10px;font-family:'JetBrains Mono',monospace">CH${ch.ch}</span>
                <span style="flex:1;display:flex;flex-wrap:wrap;gap:2px">`;
            if(offline){
                html+=`<span style="color:var(--danger);font-size:10px">OFFLINE</span>`;
            } else if(devs.length===0){
                html+=`<span style="color:var(--text-muted);font-style:italic;font-size:10px">empty</span>`;
            } else {
                html+=devs.map(a=>i2cAddrHTML(a,col)).join('');
            }
            html+=`</span></div>`;
        });
        html+=`</div>`;
    });

    if(data.main_bus){
        const mb=data.main_bus.devices||[];
        html+=`<div style="margin-top:8px;padding-top:8px;border-top:1px solid rgba(255,255,255,.08)">
            <div style="font-weight:700;color:#94a3b8;margin-bottom:7px;font-size:12px">■ Main Bus (direct)</div>
            <div style="display:flex;flex-wrap:wrap;gap:3px">
            ${mb.length?mb.map(a=>i2cAddrHTML(a,'#94a3b8')).join(''):`<span style="color:var(--text-muted);font-style:italic;font-size:11px">No devices</span>`}
            </div></div>`;
    }

    res.innerHTML=html;
    res.dataset.raw=JSON.stringify(data,null,2);
}

// ── Discovery scan: try all known addresses on every MUX/CH ──
async function runDiscovery(){
    const btn=$('disc-btn'), res=$('disc-result');
    if(!res) return;
    if(btn){ btn.disabled=true; btn.textContent='⏳ Discovering…'; }
    res.innerHTML=`<div style="border-top:1px solid rgba(255,255,255,.1);padding-top:10px;margin-top:4px">
        <div style="font-size:12px;font-weight:700;color:#f59e0b;margin-bottom:8px">🔍 Discovery Scan</div>
        <div style="color:var(--text-dim);font-size:11px">Querying /scan_json for all addresses…</div></div>`;

    let scanData;
    try { scanData = await fetch('/scan_json').then(r=>r.json()); }
    catch(e){ res.innerHTML=`<div style="color:var(--danger)">Discovery failed: ${e}</div>`; if(btn){btn.disabled=false;btn.textContent='🔍 Discovery';} return; }

    // Build flat map: mux+ch -> [addr]
    const found = {}; // key=addr -> [{mux,ch}]
    if(scanData.muxes) scanData.muxes.forEach(mux=>{
        if(mux.channels) mux.channels.forEach(ch=>{
            (ch.devices||[]).forEach(a=>{
                if(!found[a]) found[a]=[];
                found[a].push({mux:mux.addr, ch:ch.ch});
            });
        });
    });

    // Compare against expected map
    let html=`<div style="border-top:1px solid rgba(255,255,255,.1);padding-top:10px;margin-top:4px">
        <div style="font-size:12px;font-weight:700;color:#f59e0b;margin-bottom:8px">🔍 Discovery Report</div>`;

    let issues=0;
    EXPECTED_MAP.forEach(exp=>{
        const locs=found[exp.addr]||[];
        const onExpected=locs.some(l=>l.mux===exp.mux&&l.ch===exp.ch);
        const onOther=locs.filter(l=>!(l.mux===exp.mux&&l.ch===exp.ch));
        if(!onExpected){
            issues++;
            if(onOther.length){
                html+=`<div class="disc-item disc-found">
                    <span style="color:#f59e0b;font-weight:700">⚠ ${exp.sensor}</span>
                    <span style="color:var(--text-dim)"> expected at MUX ${exp.mux} CH${exp.ch} — </span>
                    <span style="color:#22c55e">found at: ${onOther.map(l=>`MUX ${l.mux} CH${l.ch}`).join(', ')}</span>
                    <div style="color:var(--text-muted);font-size:10px;margin-top:2px">Address ${exp.addr} (${I2C_DB[exp.addr]?I2C_DB[exp.addr].desc:'?'})</div>
                </div>`;
            } else {
                html+=`<div class="disc-item disc-miss">
                    <span style="color:#ef4444;font-weight:700">✗ ${exp.sensor}</span>
                    <span style="color:var(--text-dim)"> not found anywhere</span>
                    <div style="color:var(--text-muted);font-size:10px;margin-top:2px">Expected: MUX ${exp.mux} CH${exp.ch} @ ${exp.addr}</div>
                </div>`;
            }
        }
    });

    // Find unexpected addresses (not in EXPECTED_MAP)
    const expectedAddrs=EXPECTED_MAP.map(e=>e.addr);
    const muxAddrs=['0x70','0x71','0x72'];
    let unexpected=[];
    Object.keys(found).forEach(addr=>{
        if(muxAddrs.includes(addr)||addr==='0x50') return;
        if(!expectedAddrs.includes(addr)){
            found[addr].forEach(loc=>{
                const db=I2C_DB[addr];
                unexpected.push({addr,loc,name:db?db.name:'Unknown',desc:db?db.desc:'Not in database'});
            });
        }
    });
    if(unexpected.length){
        html+=`<div style="margin-top:8px;font-size:11px;font-weight:700;color:#a78bfa">🆕 Unexpected / unassigned devices:</div>`;
        unexpected.forEach(u=>{
            html+=`<div class="disc-item" style="border-left:3px solid #a78bfa;margin-top:4px">
                <span style="color:#a78bfa;font-weight:700">${u.addr}</span>
                <span style="color:var(--text-dim)"> @ MUX ${u.loc.mux} CH${u.loc.ch}</span>
                <span style="color:var(--text);margin-left:6px">${u.name}</span>
                <div style="color:var(--text-muted);font-size:10px;margin-top:2px">${u.desc}</div>
            </div>`;
        });
    }

    if(!issues && !unexpected.length){
        html+=`<div style="color:#22c55e;font-size:11px;padding:8px 0">✓ All expected sensors found on correct MUX/CH. No anomalies detected.</div>`;
    }

    html+=`<div style="font-size:10px;color:var(--text-muted);margin-top:8px;text-align:right">${Object.values(found).flat().length} device-slots found · ${EXPECTED_MAP.length} expected</div></div>`;
    res.innerHTML=html;
    if(btn){ btn.disabled=false; btn.textContent='🔍 Discovery'; }
}

function copyI2CScan(){
    const res=$('i2c-result'); if(!res) return;
    const raw=res.dataset.raw;
    if(raw) navigator.clipboard?.writeText(raw).then(()=>showToast('Scan copied (JSON)'));
}

// ═══════════════════════════════════════════════
// LOGS — wide, scrollable, copy on select
// ═══════════════════════════════════════════════
let logsRunning=false;
function loadLogs(){
    const vw=window.innerWidth, w=Math.min(vw-16,900);
    createWindow('log-win','Live Log Stream','📜',30,40,`
        <div class="log-toolbar">
            <input class="log-search" id="logSearch" placeholder="🔍 Filter log…" oninput="filterLog()">
            <button class="btn accent" onclick="clearLog()">Clear</button>
            <button class="btn" onclick="scrollLogBottom()">⬇ End</button>
            <button class="btn" onclick="pasteLog()">📋 Copy All</button>
        </div>
        <div id="log-terminal" onmouseup="autoSelectCopy()"></div>
    `,w);
    logsRunning=true;
    pollLogs();
}

let _lastLogText='';
function pollLogs(){
    if(!$('log-terminal')){ logsRunning=false; return; }
    fetch('/logs').then(r=>r.text()).then(t=>{
        const term=$('log-terminal'); if(!term) return;
        if(t!==_lastLogText){
            _lastLogText=t;
            const q=($('logSearch')||{}).value||'';
            renderLog(t,q);
        }
    }).catch(()=>{});
    if(logsRunning) setTimeout(pollLogs,1500);
}

function renderLog(txt,q){
    const term=$('log-terminal'); if(!term) return;
    const wantScroll=(term.scrollHeight-term.scrollTop-term.clientHeight)<60;
    if(q){
        const re=new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi');
        term.innerHTML=txt.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
            .replace(re,m=>`<mark style="background:rgba(0,212,170,.3);color:#fff;border-radius:2px">${m}</mark>`);
    } else {
        term.textContent=txt;
    }
    if(wantScroll) term.scrollTop=term.scrollHeight;
}
function filterLog(){ if(_lastLogText) renderLog(_lastLogText,($('logSearch')||{}).value||''); }
function clearLog(){ const t=$('log-terminal'); if(t){t.textContent=''; _lastLogText='';} }
function scrollLogBottom(){ const t=$('log-terminal'); if(t) t.scrollTop=t.scrollHeight; }
function pasteLog(){
    const t=$('log-terminal'); if(!t) return;
    navigator.clipboard?.writeText(t.innerText||t.textContent).then(()=>showToast('Log copied!'));
}
function autoSelectCopy(){
    const sel=window.getSelection();
    if(sel&&sel.toString().length>0){
        navigator.clipboard?.writeText(sel.toString()).then(()=>showToast('Copied selection'));
    }
}

function uploadOTA(){
    const f=$('otafile'); if(!f||!f.files[0]) return notify('Select .bin first',false);
    const fd=new FormData(); fd.append('update',f.files[0]);
    const xhr=new XMLHttpRequest(); xhr.open('POST','/update',true);
    xhr.upload.onprogress=e=>{if(e.lengthComputable) $('ota-prog').textContent=(e.loaded/e.total*100).toFixed(1)+'%';};
    xhr.onload=()=>{$('ota-prog').textContent=xhr.status===200?'✓ SUCCESS — rebooting…':'✗ FAILED';};
    xhr.send(fd);
}

// ═══════════════════════════════════════════════════════════════════
// 🏥 MEDICAL DASHBOARD – human-readable, risk-oriented
// ═══════════════════════════════════════════════════════════════════
// ── Risk card (hero version for Medical panel) ──
function riskCard(icon,label,val,unit,cat,warnAt,dangerAt,maxVal,desc){
    const n = (val!=null&&val!==undefined) ? parseFloat(val) : null;
    const safe = n!=null && Number.isFinite(n);
    const st = safe ? (n>=dangerAt?'danger':n>=warnAt?'warn':'good') : 'good';
    const col = st==='danger'?'#ef4444':st==='warn'?'#f59e0b':'#22c55e';
    const bg  = st==='danger'?'rgba(239,68,68,.07)':st==='warn'?'rgba(245,158,11,.07)':'rgba(34,197,94,.05)';
    const pct = safe ? Math.min(100,(n/maxVal)*100) : 0;
    const fillGrad = pct>70
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 40%,#ef4444)'
        : pct>40 ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)';
    const warnPct = Math.min(100,(warnAt/maxVal)*100);
    const danPct  = Math.min(100,(dangerAt/maxVal)*100);
    return `<div style="background:${bg};border-radius:12px;padding:14px 15px;
                        border:1px solid rgba(255,255,255,.07);border-left:3px solid ${col};
                        margin-bottom:8px">
        <div style="display:flex;align-items:center;gap:10px;margin-bottom:8px">
            <span style="font-size:20px">${icon}</span>
            <div style="flex:1">
                <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;
                    letter-spacing:.7px;font-weight:700">${label}</div>
                ${desc?`<div style="font-size:9px;color:var(--text-muted);opacity:.7;margin-top:1px">${desc}</div>`:''}
            </div>
            <div style="text-align:right">
                <div style="font-size:24px;font-weight:800;color:${col};line-height:1;
                    font-family:'JetBrains Mono',monospace">${safe?fv(n,n<10?1:0):'--'}
                    <span style="font-size:11px;font-weight:400;color:var(--text-dim)">${unit}</span>
                </div>
                ${cat?`<div style="font-size:10px;color:${col};font-weight:600;margin-top:2px">${cat}</div>`:''}
            </div>
        </div>
        <div style="position:relative;height:6px">
            <div style="width:100%;height:100%;border-radius:999px;background:rgba(255,255,255,.07)"></div>
            <div style="position:absolute;left:0;top:0;height:100%;border-radius:999px;
                background:${fillGrad};width:${pct}%;transition:width .7s cubic-bezier(.4,0,.2,1);
                box-shadow:0 0 8px ${col}55"></div>
            <div style="position:absolute;top:-2px;bottom:-2px;width:2px;border-radius:1px;
                left:${warnPct}%;background:rgba(245,158,11,.8)"></div>
            <div style="position:absolute;top:-2px;bottom:-2px;width:2px;border-radius:1px;
                left:${danPct}%;background:rgba(239,68,68,.9)"></div>
        </div>
    </div>`;
}

function loadMedical(){
    const vw=window.innerWidth, w=Math.min(vw-16,520);
    createWindow('med-win','🏥 '+t('medical'),'🏥',20,40,`
        <div id="medGrid" style="color:var(--text-muted);font-size:11px;text-align:center;padding:20px">
            ⏳ Loading…
        </div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('medGrid'); if(!g) return;

        // Biometeo alert banner
        const ba=s.MED_Biometeo_Alert;
        let html='';
        if(ba==='HIGH'||ba==='MODERATE'){
            const c=ba==='HIGH'?'#ef4444':'#f59e0b';
            html+=`<div style="background:${c}18;border:1px solid ${c}44;border-radius:10px;
                padding:12px 14px;margin-bottom:12px;display:flex;align-items:center;gap:10px">
                <span style="font-size:22px">${ba==='HIGH'?'🚨':'⚠️'}</span>
                <div>
                    <div style="font-size:12px;font-weight:700;color:${c}">${t('biometeo')}: ${ba}</div>
                    <div style="font-size:10px;color:var(--text-dim)">Score: ${s.MED_Biometeo_Score!=null?(+s.MED_Biometeo_Score).toFixed(1)+'/10':''}</div>
                </div>
            </div>`;
        }

        // Section: Pain & Pressure Sensitivity
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:0 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            🩺 Pain & Pressure Sensitivity</div>`;
        html+=riskCard('🧠',t('migraine'),s.MED_Migraine_Risk,'/10',s.MED_Migraine_Cat,5,7,10,'Barometric + humidity driver');
        html+=riskCard('🦴',t('rheum'),s.MED_Rheumatic_Risk,'/10',s.MED_Rheumatic_Cat,5,7,10,'Joint & tissue pressure sensitivity');
        html+=riskCard('📊',t('bpi'),s.MED_Baro_Pain_Index,'/10',null,6,8,10,'Shutty chronic pain index');
        html+=riskCard('👃',t('sinus'),s.MED_Sinus_Risk,'/10',null,4,7,10,'Pressure delta + humidity');

        // Section: Respiratory
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:12px 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            🫁 Respiratory & Air Quality</div>`;
        html+=riskCard('🫁',t('lung')+' Rest',s.MED_Lung_Deposit_Rest,'µg/min',null,0.3,0.8,2,'ICRP alveolar deposition model');
        html+=riskCard('🫁',t('lung')+' Exercise',s.MED_Lung_Deposit_Exer,'µg/min',null,0.8,2.0,5,'During physical activity');
        html+=riskCard('🌿','IAQ Score',s.GAS_IAQ_Score,'/100',s.GAS_Toxicity_Name,
            s.GAS_IAQ_Score>50?50:s.GAS_IAQ_Score,25,100,'BSEC AI indoor air quality');
        html+=riskCard('💨','Gas Toxicity',s.GAS_Toxicity_Risk,'/10',s.GAS_Toxicity_Name,5,7,10,'Pattern-based gas hazard');
        html+=riskCard('💧','Required ACH',s.MED_Required_ACH,'/h',null,3,8,12,'Air changes/h needed for CO₂ dilution');

        // Section: Radiation & Environment
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:12px 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            ☢️ Radiation & Thermal</div>`;
        
        html+=riskCard('🌡️',t('utci'),s.MED_UTCI_C,'°C',s.MED_UTCI_Cat,-5,32,50,'Universal Thermal Climate Index');
        html+=riskCard('🏠',t('radon'),s.MED_Radon_Risk,'/10',null,5,7.5,10,'Radon accumulation proxy');

        g.style.display='block'; g.innerHTML=html;
    }).catch(()=>{ const g=$('medGrid'); if(g) g.innerHTML='<div style="color:var(--danger)">Connection error</div>'; });
}

// ═══════════════════════════════════════════════════════════════════
// 🌌 SPACE WEATHER DASHBOARD
// ═══════════════════════════════════════════════════════════════════
function loadSpaceWeather(){
    const vw=window.innerWidth, w=Math.min(vw-16,520);
    createWindow('space-win','🌌 '+t('space'),'🌌',40,40,`
        <div id="spaceGrid"><div style="color:var(--text-muted);padding:20px;text-align:center">⏳ Loading…</div></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('spaceGrid'); if(!g) return;

        const k=+s.SPACE_K_Index_Proxy||0;
        const kcolor = k>=7?'#ff4757':k>=5?'#ffa502':k>=3?'#f59e0b':'#4ade80';
        const fb=+s.SPACE_Forbush_Pct||0;
        const aur=+s.SPACE_Aurora_Prob_Pct||0;
        const dbdt=+s.SPACE_dBdt_nTs||0;

        let html='';

        // K-index big display
        html+=`<div style="background:${kcolor}18;border:2px solid ${kcolor}55;border-radius:14px;
                           padding:18px;text-align:center;margin-bottom:10px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px">Local K-Index (proxy)</div>
            <div style="font-size:56px;font-weight:800;color:${kcolor};line-height:1;margin:6px 0">${k>=0?k:'?'}</div>
            <div style="font-size:12px;color:${kcolor}">${s.SPACE_SSC_Class||'Quiet'}</div>
            <div style="font-size:10px;color:var(--text-muted);margin-top:4px">dB/dt: ${fv(s.SPACE_dBdt_nTs,2)} nT/s &nbsp;|&nbsp; Dev: ${fvi(s.SPACE_B_Dev_nT)} nT</div>
        </div>`;

        // Aurora
        const aurColor = aur>50?'#818cf8':aur>10?'#a78bfa':'#4ade80';
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
                <div style="font-size:12px;font-weight:600;color:var(--text)">🌌 ${t('aurora')}</div>
                <div style="font-size:22px;font-weight:800;color:${aurColor}">${fv(s.SPACE_Aurora_Prob_Pct,1)}%</div>
            </div>
            <div style="background:rgba(255,255,255,.07);border-radius:4px;height:8px;overflow:hidden;margin-bottom:8px">
                <div style="width:${Math.min(100,aur)}%;height:100%;background:${aurColor};border-radius:4px;transition:width .5s"></div>
            </div>
            <div style="font-size:10px;color:var(--text-muted)">
                Sky: <span style="color:var(--text)">${s.MLX_Sky_Condition||'Unknown'}</span> &nbsp;|&nbsp;
                Clear window: <span style="color:${s.SPACE_Aurora_Sky_Clear==='YES'?'var(--accent)':'var(--danger)'}">${s.SPACE_Aurora_Sky_Clear||'?'}</span>
            </div>
        </div>`;

        // Forbush
        const fbColor = fb<-8?'#ff4757':fb<-5?'#ffa502':fb<-2?'#f59e0b':'#4ade80';
        html+=bigCard('☢️',t('forbush'),fv(s.SPACE_Forbush_Pct,1)+'%',
            s.SPACE_Forbush_Class||'Normal',null,fbColor,fb<-5?'danger':fb<-2?'warn':'good');

        // Ozone
        html+=bigCard('🔵',t('ozone'),fvi(s.SPACE_Ozone_DU||s.SPACE_Ozone_DU_Proxy),'DU',
            'Stratospheric proxy','#38bdf8','good');

        

        // Magnetic field
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px">🧲 Magnetic Field (BMM350)</div>
            <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;text-align:center">
                ${['X','Y','Z'].map((ax,i)=>{
                    const v=[s.BMM350_Mag_X,s.BMM350_Mag_Y,s.BMM350_Mag_Z][i];
                    return `<div style="background:rgba(255,255,255,.05);border-radius:8px;padding:8px">
                        <div style="font-size:9px;color:var(--text-muted)">${ax} [µT]</div>
                        <div style="font-size:16px;font-weight:700;color:#818cf8">${fv(v,2)}</div>
                    </div>`;
                }).join('')}
            </div>
            <div style="text-align:center;margin-top:8px;font-size:13px;color:var(--text)">
                |B| = <strong style="color:#818cf8">${fv(s.SPACE_B_Total_uT,2)} µT</strong> &nbsp;
                Heading: <strong>${fv(s.BMM350_Heading_Deg,1)}° ${s.BMM350_Cardinal||''}</strong>
            </div>
        </div>`;

        // Solar AOD
        html+=bigCard('🌞',t('aod'),fv(s.ASTRO_AOD,3),'',
            'Aerosol Optical Depth','#fbbf24',+s.ASTRO_AOD>0.4?'warn':'good');

        g.innerHTML=html;
    }).catch(()=>{ const g=$('spaceGrid'); if(g) g.innerHTML='<div style="color:var(--danger)">Error</div>'; });
}

// ═══════════════════════════════════════════════════════════════════
// 🌿 ENVIRONMENTAL DASHBOARD
// ═══════════════════════════════════════════════════════════════════
function loadEnviron(){
    const vw=window.innerWidth, w=Math.min(vw-16,560);
    createWindow('env2-win','🌿 '+t('environ'),'🌿',50,40,`
        <div id="envGrid"><div style="color:var(--text-muted);padding:20px;text-align:center">⏳</div></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('envGrid'); if(!g) return;
        let html='';

        // Weather headline
        html+=`<div style="background:linear-gradient(135deg,rgba(0,212,170,.12),rgba(56,189,248,.08));
                           border:1px solid rgba(0,212,170,.2);border-radius:14px;padding:16px;margin-bottom:10px">
            <div style="display:flex;justify-content:space-between;align-items:flex-start">
                <div>
                    <div style="font-size:36px;font-weight:800;color:var(--accent)">${fv(s.SHT45_Temp,1)}°C</div>
                    <div style="font-size:13px;color:var(--text-dim)">Feels ${fv(s.METEO_Feels_Like_C,1)}°C &nbsp; ${t('hum')}: ${fv(s.SHT45_Hum,0)}%</div>
                </div>
                <div style="text-align:right">
                    <div style="font-size:14px;color:var(--text-dim)">${s.MLX_Sky_Condition||'--'}</div>
                    <div style="font-size:11px;color:var(--text-muted)">${fv(s.METEO_Sea_Level_Press_hPa,1)} hPa (SLP)</div>
                    <div style="font-size:11px;color:var(--text-muted)">Zambretti: ${s.METEO_Zambretti_Forecast||'--'}</div>
                </div>
            </div>
        </div>`;

        // Air quality section
        const eaqi=+(s.AIR_EAQI_Index || s.AIR_EPA_AQI)||0;
        const eaqiCol=eaqi>=5?'#ff4757':eaqi>=4?'#f97316':eaqi>=3?'#ffa502':eaqi>=2?'#facc15':'#4ade80';
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="display:flex;align-items:center;gap:12px">
                <div style="background:${eaqiCol}22;border-radius:50%;width:52px;height:52px;display:flex;align-items:center;
                            justify-content:center;font-size:22px;font-weight:800;color:${eaqiCol};flex-shrink:0">${eaqi}</div>
                <div>
                    <div style="font-size:12px;font-weight:600;color:var(--text)">${s.AIR_Quality_Status||'Unknown'}</div>
                    <div style="font-size:10px;color:var(--text-muted)">EAQI &nbsp;|&nbsp; WHO: ${fv(s.AIR_WHO_AQI_Pct,0)}% &nbsp;|&nbsp; IAQ: ${fvi(s.BME688_IAQ)}</div>
                    <div style="font-size:10px;color:var(--text-muted)">Visibility: ${fv(s.AIR_Visibility_Km,1)} km &nbsp;|&nbsp; Smog: ${fv(s.AIR_Smog_Index_Advanced,1)}</div>
                </div>
            </div>
        </div>`;

        // PM + Gas grid
        html+=`<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:8px">`;
        [
            ['🌫️','PM1.0',s.BMV080_PM1_0,'µg/m³',0],
            ['🌫️','PM2.5',s.BMV080_PM2_5,'µg/m³',35],
            ['🌫️','PM10',s.BMV080_PM10_0,'µg/m³',50],
            ['🫧','CO₂ (NDIR)',s.SCD41_CO2_ppm,'ppm',1000],
            ['💨','VOC Index',s.SGP41_VOC_Index,'',150],
            ['🔬','NOx Index',s.SGP41_NOx_Index,'',20],
            ['🧪','NO2',s.ZMOD4510_NO2_ppb,'ppb',50],
            ['🌞','O3',s.ZMOD4510_O3_ppb,'ppb',70],
        ].forEach(([ic,lb,val,un,thr])=>{
            if(val==null) return;
            const n=+val; const col=n>thr?'#ffa502':'#4ade80';
            html+=`<div style="background:rgba(0,0,0,.22);border-radius:10px;padding:10px">
                <div style="font-size:9px;color:var(--text-muted)">${ic} ${lb}</div>
                <div style="font-size:20px;font-weight:700;color:${col}">${fv(val,1)}</div>
                <div style="font-size:9px;color:var(--text-dim)">${un}</div>
            </div>`;
        });
        html+=`</div>`;

        // Weather details
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px">🌤 Meteorological Derived</div>
            <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:11px">
                ${[
                    ['Dew Point',s.METEO_Dew_Point_C,'°C'],
                    ['Wet Bulb',s.METEO_Wet_Bulb_C,'°C'],
                    ['Cloud Base',s.METEO_Cloud_Base_m,'m'],
                    ['LCL Alt.',s.METEO_LCL_m,'m'],
                    ['Air Density',s.METEO_Air_Density,'kg/m³'],
                    ['θe Instab.',s.METEO_ThetaE_K,'K'],
                    ['Conv. Instab.',s.METEO_Conv_Instability,'/10'],
                    ['GHI Solar',s.METEO_Solar_GHI_Wm2,'W/m²'],
                    ['Precipitable H₂O',s.METEO_PW_mm,'mm'],
                    ['Frost Risk',s.METEO_Frost_Risk_Pct,'%'],
                    ['Zambretti',s.METEO_Zambretti_Forecast,''],
                    ['Fire Risk',s.ENV_Fire_Risk_Pct,'%'],
                ].map(([lb,val,un])=>val!=null?
                    `<div style="background:rgba(255,255,255,.04);border-radius:6px;padding:6px 8px">
                        <div style="color:var(--text-muted);font-size:9px">${lb}</div>
                        <div style="color:var(--text);font-weight:600">${fv(val,1)} <span style="color:var(--text-dim);font-size:9px">${un}</span></div>
                    </div>`:'').join('')}
            </div>
        </div>`;

        // Wind (from WU)
        if(s.WIND_Speed_Kph!=null){
            const dir=+s.WIND_Direction_Deg||0;
            html+=bigCard('💨','Wind',fv(s.WIND_Speed_Kph,1)+' km/h',
                `→ ${s.WIND_Speed_Kph}°`, `Gust: ${fv(s.WIND_Gust_Kph,1)} km/h`,'#38bdf8','good');
        }

        // UV
        const uvi=+s.LTR390_UVI||0;
        html+=bigCard('☀️',t('uvi'),fv(s.LTR390_UVI,1),'',
            `${getUVName(uvi)} | VD: ${fv(s.MED_VitD_Time_min,0)} min`,'#f59e0b',
            uvi>8?'danger':uvi>3?'warn':'good');

        g.innerHTML=html;
    });
}
function getUVName(v){ return v<3?'Low':v<6?'Moderate':v<8?'High':v<11?'Very High':'Extreme'; }

// ═══════════════════════════════════════════════════════════════════
// 🌈 OPTICAL SPECTROMETER
// ═══════════════════════════════════════════════════════════════════
function loadSpectrometer(){
    const vw=window.innerWidth, w=Math.min(vw-16,540);
    createWindow('spec-win','🌈 Optical Spectrometer','🌈',30,40,`
        <div style="font-size:10px;color:var(--text-muted);text-align:center;margin-bottom:8px">
            Spectral & UV Analysis &nbsp;|&nbsp; AS7343 (14-ch) + AS7331 (UVA/B/C)
        </div>
        <div style="background:rgba(0,0,0,.3);border-radius:10px;padding:12px;margin-bottom:8px">
            <canvas id="spec-canvas" style="width:100%;height:120px;display:block"></canvas>
            <div id="spec-gain" style="text-align:right;font-size:10px;color:var(--text-muted);margin-top:4px"></div>
        </div>
        <div id="spec-uv" style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-bottom:8px"></div>
        <div id="spec-meta" style="font-size:11px"></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        renderSpectrometer(s);
    });
    setTimeout(()=>{
        fetch('/api').then(r=>r.json()).then(d=>{ renderSpectrometer(d.sensors||{}); });
    },10000);
}

function renderSpectrometer(s){
    const cvs=$('spec-canvas'); if(!cvs) return;
    // AS7343 channels: wavelength order
    const channels = [
        {k:'AS7343_F1_415nm', wl:415, label:'415', col:'#8b5cf6'},
        {k:'AS7343_FZ_450nm', wl:450, label:'450', col:'#6366f1'},
        {k:'AS7343_F2_445nm', wl:445, label:'445', col:'#4f46e5'},
        {k:'AS7343_F3_480nm', wl:480, label:'480', col:'#2563eb'},// 480 blue
        {k:'AS7343_F4_515nm', wl:515, label:'515', col:'#0891b2'},
        {k:'AS7343_FY_555nm', wl:555, label:'555', col:'#16a34a'},
        {k:'AS7343_F5_555nm', wl:555, label:'555b',col:'#22c55e'},
        {k:'AS7343_FXL_600nm',wl:600, label:'600', col:'#ca8a04'},
        {k:'AS7343_F6_640nm', wl:640, label:'640', col:'#d97706'},
        {k:'AS7343_F7_680nm', wl:680, label:'680', col:'#dc2626'},
        {k:'AS7343_F8_910nm', wl:910, label:'910', col:'#be185d'},
    ];

    // Normalize to max
    const vals = channels.map(c=>+(s[c.k]||0));
    const maxVal = Math.max(...vals,1);

    // Draw bars on canvas
    const W=cvs.offsetWidth||480, H=cvs.offsetHeight||120;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    ctx.clearRect(0,0,W,H);

    const barW=Math.floor(W/channels.length)-2;
    channels.forEach((c,i)=>{
        const v=vals[i]; const h=Math.ceil((v/maxVal)*(H-24));
        const x=i*(barW+2)+1;
        // Gradient bar
        const grad=ctx.createLinearGradient(0,H-h,0,H);
        grad.addColorStop(0,c.col+'ff');
        grad.addColorStop(1,c.col+'55');
        ctx.fillStyle=grad;
        ctx.beginPath();
        // Pill shape
        const r=Math.min(barW/2,6);
        ctx.roundRect?ctx.roundRect(x,H-h-16,barW,h,r):ctx.rect(x,H-h-16,barW,h);
        ctx.fill();
        // Label
        ctx.fillStyle='rgba(255,255,255,.5)';
        ctx.font='8px Inter,sans-serif';
        ctx.textAlign='center';
        ctx.fillText(c.label,x+barW/2,H-4);
    });

    // Gain label
    const g=$('spec-gain'); if(g) g.textContent=`GAIN: ${fv(s.AS7343_Gain,1)}X`;

    // UV section (AS7331)
    const uvDiv=$('spec-uv'); if(uvDiv){
        uvDiv.innerHTML=[
            ['UVA','☀️',s.AS7331_UVA,'#fde047'],
            ['UVB','⚡',s.AS7331_UVB,'#fb923c'],
            ['UVC','🔬',s.AS7331_UVC,'#a78bfa'],
        ].map(([lb,ic,val,col])=>`
            <div style="background:rgba(0,0,0,.3);border-radius:8px;padding:10px;text-align:center">
                <div style="font-size:9px;color:var(--text-muted)">${ic} ${lb}</div>
                <div style="font-size:18px;font-weight:700;color:${col}">${fv(val,2)}</div>
                <div style="font-size:9px;color:var(--text-dim)">µW/cm²</div>
            </div>`).join('');
    }

    // Meta info
    const meta=$('spec-meta'); if(meta) meta.innerHTML=`
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:11px">
            ${[
                ['Medical UVI',fv(s.AS7331_Medical_UVI,2),'','#f59e0b'],
                ['Clear',fvi(s.AS7343_Clear),'ADC','#94a3b8'],
                ['NIR 910nm',fvi(s.AS7343_nIR),'ADC','#be185d'],
                ['TSL2591 Lux',fvi(s.TSL2591_Lux),'lx','#fde68a'],
                ['OPT4048 Lux',fv(s.OPT4048_Lux,1),'lx','#fde68a'],
                ['Melanopic',fv(s.OPTICS_Melanopic_Lux,1),'mlux','#c084fc'],
            ].map(([lb,v,un,c])=>`
                <div style="background:rgba(255,255,255,.04);border-radius:6px;padding:6px 8px">
                    <div style="font-size:9px;color:var(--text-muted)">${lb}</div>
                    <div style="font-weight:600;color:${c}">${v} <span style="font-size:9px;color:var(--text-dim)">${un}</span></div>
                </div>`).join('')}
        </div>
        ${s.OPTICS_Melatonin_Status?`<div style="margin-top:8px;font-size:11px;color:var(--text-dim)">
            🌙 Melatonin: <strong style="color:var(--accent)">${s.OPTICS_Melatonin_Status}</strong></div>`:''}`;
}



// ── Pre-populate history so charts work on revisit ──
loadSection('dash');
</script>
</body>
</html>
)rawliteral";

#endif // WWW_INDEX_H
