#ifndef WEB_PAGE_HPP
#define WEB_PAGE_HPP

#include <cstddef>

namespace web {
inline constexpr const char kIndexHtml[] = R"WEBPAGE(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="utf-8">
    <title>Servidor IoT</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body{font-family:Arial,Helvetica,sans-serif;background:#A2B2BC;color:#fff;margin:0;display:flex;align-items:center;justify-content:center;min-height:100vh}
        .card{background:rgba(0,0,0,.1);padding:20px 24px;border-radius:12px;text-align:center;width:92%;max-width:640px}
        h1{margin:6px 0 10px}
        p{margin:0 0 16px;color:#eef}
        .row{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;margin-top:10px}
        input[type=text]{flex:1;min-width:220px;padding:10px 12px;border-radius:8px;border:none;outline:none}
        .btn{background:#ff6b6b;border:none;padding:10px 16px;border-radius:8px;color:#fff;font-weight:700;cursor:pointer}
        .btn.secondary{background:#4caf50}
        #msg{margin-top:10px}
    </style>
</head>
<body>
    <div class="card">
        <h1>Servidor IoT</h1>
        <p>Raspberry Pi Pico + ESP8266</p>
        <div class="row">
            <form method="GET" action="/buzzer">
                <button class="btn" type="submit">Beep</button>
            </form>
        </div>
        <div class="row">
            <form method="GET" action="/morse" style="display:flex;gap:8px;align-items:center">
                <input type="text" name="text" maxlength="64" placeholder="Texto a Morse (ej. SOS)" required>
                <button class="btn secondary" type="submit">Enviar Morse</button>
            </form>
        </div>
        <div id="msg"></div>
    </div>
</body>
</html>
)WEBPAGE";

inline constexpr std::size_t kIndexHtmlLen = sizeof(kIndexHtml) - 1;
inline constexpr const char kIndexContentType[] = "text/html; charset=utf-8";
} // namespace web

#endif // WEB_PAGE_HPP
