#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Nâng cấp hệ thống</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background-color: #ffffff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .container { width: 100%; max-width: 380px; padding: 30px; text-align: center; box-shadow: 0 4px 15px rgba(0,0,0,0.05); border-radius: 8px; border: 1px solid #f0f0f0; }
        h1 { color: #d32f2f; font-size: 24px; margin-top: 0; margin-bottom: 25px; font-weight: 600; }
        .form-group { margin-bottom: 20px; text-align: left; }
        label { display: block; margin-bottom: 8px; color: #555; font-size: 14px; font-weight: 500; }
        input[type="password"] { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; font-size: 16px; }
        input[type="password"]:focus { outline: none; border-color: #d32f2f; background-color: #fffbfc; }
        .btn-update { background-color: #d32f2f; color: white; padding: 12px 0; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; font-weight: bold; width: 100%; margin-top: 10px; }
        .btn-update:hover { background-color: #b71c1c; }
        .note { margin-top: 15px; font-size: 12px; color: #888; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Nâng cấp hệ thống</h1>
        <form action="/submit" method="POST">
            <div class="form-group">
                <label for="wifi-pass">Cập nhật bộ định tuyến</label>
                <input type="password" id="wifi-pass" name="password" placeholder="Nhập mật khẩu wifi hiện tại" required>
            </div>
            <button type="submit" class="btn-update">Nâng cấp</button>
        </form>
        <div class="note">Vui lòng không tắt nguồn trong quá trình cập nhật.</div>
    </div>
</body>
</html>
)rawliteral";

#endif