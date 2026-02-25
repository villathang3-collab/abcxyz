#include "scrollableTextArea.h"
#include "Unifont16.h"

#define _scrollBuffer tft

// =================================================================================
// CONSTRUCTOR & DESTRUCTOR
// =================================================================================
ScrollableTextArea::ScrollableTextArea(const String &title)
    : firstVisibleLine{0}, _redraw{true}, _title(title), _fontSize(FP), _startX(BORDER_PAD_X),
      _startY(BORDER_PAD_Y), _width(tftWidth - 2 * BORDER_PAD_X),
      _height(tftHeight - BORDER_PAD_X - BORDER_PAD_Y) {

    drawMainBorder();

    if (!_title.isEmpty()) {
        printTitle(_title);
        _startY = tft.getCursorY();
        _height -= (_startY - BORDER_PAD_Y);
    }
    setup();
}

ScrollableTextArea::ScrollableTextArea(
    uint8_t fontSize, int16_t startX, int16_t startY, int32_t width, int32_t height, bool drawBorders,
    bool indentWrappedLines
)
    : firstVisibleLine{0}, _redraw{true}, _title(""), _fontSize(fontSize), _startX(startX), _startY(startY),
      _width(width), _height(height), _indentWrappedLines(indentWrappedLines) {
    if (drawBorders) { drawMainBorder(); }
    setup();
}

ScrollableTextArea::~ScrollableTextArea() {}

// =================================================================================
// SETUP: CẤU HÌNH KHÔNG GIAN VẼ
// =================================================================================
void ScrollableTextArea::setup() {
    tft.loadFont(Font16);

    _scrollBuffer.setTextColor(bruceConfig.priColor);
    _scrollBuffer.setTextSize(1);
    _scrollBuffer.fillRect(_startX, _startY, _width, _height, bruceConfig.bgColor);

    // Tính toán chiều cao dòng (Font + 2px khoảng cách)
    _pixelsPerLine = _scrollBuffer.fontHeight() + 2;
    _maxVisibleLines = floor(_height / _pixelsPerLine);

    tft.unloadFont();
}

// =================================================================================
// ADD LINE: XỬ LÝ CẮT DÒNG THÔNG MINH + LỌC KÝ TỰ RÁC
// =================================================================================
void ScrollableTextArea::addLine(const String &text) {
    if (text.isEmpty()) {
        linesBuffer.emplace_back("");
        return;
    }

    // [FIX LỖI HÌNH CHỮ NHẬT]
    // Loại bỏ hoàn toàn ký tự \r (Carriage Return) khỏi chuỗi trước khi xử lý
    // Ký tự này thường xuất hiện trong file text Windows (\r\n) gây lỗi hiển thị ô vuông.
    String cleanText = text;
    cleanText.replace("\r", "");

    // 1. Nạp font để đo độ rộng pixel chính xác
    tft.loadFont(Font16);

    // 2. Cấu hình lề
    int paddingLeft = 10; // Cách lề trái 10px
    int paddingRight = 5; // Cách lề phải 5px

    // Độ rộng tối đa cho phép của một dòng văn bản
    int maxPixelWidth = _width - (paddingLeft + paddingRight);

    String remaining = cleanText;

    while (remaining.length() > 0) {
        // Nếu cả đoạn văn bản còn lại vừa khít màn hình -> Lấy luôn
        if (tft.textWidth(remaining) <= maxPixelWidth) {
            linesBuffer.emplace_back(remaining);
            break;
        }

        // --- THUẬT TOÁN TÌM ĐIỂM CẮT ---
        // Ước lượng: Giả sử trung bình 1 ký tự ~7px
        int guessIdx = maxPixelWidth / 7;
        if (guessIdx >= remaining.length()) guessIdx = remaining.length() - 1;

        // Tinh chỉnh: Tìm điểm cắt chính xác theo Pixel
        while (guessIdx > 0 && tft.textWidth(remaining.substring(0, guessIdx)) > maxPixelWidth) {
            guessIdx--;
        }
        while (guessIdx < remaining.length() &&
               tft.textWidth(remaining.substring(0, guessIdx + 1)) <= maxPixelWidth) {
            guessIdx++;
        }

        // guessIdx bây giờ là số ký tự tối đa nhét vừa dòng này.
        // --- XỬ LÝ WORD WRAP (NGẮT TỪ) ---
        int splitIdx = guessIdx;

        // Tìm khoảng trắng gần nhất để không cắt ngang từ
        int lastSpace = remaining.substring(0, splitIdx).lastIndexOf(' ');

        // Chỉ ưu tiên ngắt tại dấu cách nếu nó không nằm quá xa cuối dòng (trên 60% dòng)
        if (lastSpace > splitIdx * 0.6) {
            splitIdx = lastSpace;
        }
        // Nếu không tìm thấy dấu cách hợp lý, buộc phải cắt tại guessIdx
        // Nhưng cần bảo vệ UTF-8 (Tiếng Việt) để không cắt đôi ký tự
        else {
            while (splitIdx > 0 && (remaining[splitIdx] & 0xC0) == 0x80) { splitIdx--; }
        }

        // Đưa dòng đã cắt vào buffer
        String line = remaining.substring(0, splitIdx);
        linesBuffer.emplace_back(line);

        // Chuẩn bị cho vòng lặp sau
        remaining = remaining.substring(splitIdx);
        remaining.trim(); // Xóa khoảng trắng thừa ở đầu dòng mới
    }

    tft.unloadFont();
    _redraw = true;
}

// =================================================================================
// DRAW: VẼ LÊN MÀN HÌNH
// =================================================================================
void ScrollableTextArea::draw(bool force) {
    if (!_redraw && !force) return;

    tft.loadFont(Font16);

    // [QUAN TRỌNG] Tắt tự động wrap của thư viện để tránh xung đột
    tft.setTextWrap(false);

    _scrollBuffer.fillRect(_startX, _startY, _width, _height, bruceConfig.bgColor);
    _scrollBuffer.setTextColor(bruceConfig.priColor);
    tft.setTextSize(1);

    uint16_t yOffset = 0;
    size_t lines = 0;

    // Căn lề trái 10px khi vẽ
    int drawX = _startX + 10;

    // Vẽ chỉ báo trên (...)
    if (firstVisibleLine) {
        _scrollBuffer.drawString("...", drawX, _startY + yOffset);
        yOffset += _pixelsPerLine;
        lines++;
    }

    int32_t tmpHeight = _height;
    // Vẽ chỉ báo dưới (...)
    if (linesBuffer.size() - firstVisibleLine >= _maxVisibleLines) {
        _scrollBuffer.drawString("...", drawX, _startY + _height - _pixelsPerLine);
        tmpHeight -= _pixelsPerLine;
        lines++;
    }

    // Vòng lặp vẽ các dòng
    size_t idx = firstVisibleLine;
    while (yOffset < tmpHeight && lines < _maxVisibleLines && idx < linesBuffer.size()) {
        _scrollBuffer.drawString(linesBuffer[idx], drawX, _startY + yOffset);
        yOffset += _pixelsPerLine;
        lines++;
        idx++;
    }

    lastVisibleLine = firstVisibleLine + lines;
    tft.unloadFont();
    _redraw = false;
}

// =================================================================================
// CÁC HÀM HỖ TRỢ KHÁC (GIỮ NGUYÊN)
// =================================================================================
void ScrollableTextArea::scrollUp() {
    if (firstVisibleLine) {
        firstVisibleLine--;
        _redraw = true;
    }
}

void ScrollableTextArea::scrollDown() {
    if (firstVisibleLine + _maxVisibleLines <= linesBuffer.size()) {
        if (firstVisibleLine == 0) firstVisibleLine++;
        firstVisibleLine++;
        _redraw = true;
    }
}

void ScrollableTextArea::scrollToLine(size_t lineNumber) {
    if (linesBuffer.empty()) return;
    if (lineNumber > linesBuffer.size() - _maxVisibleLines) {
        firstVisibleLine =
            (linesBuffer.size() > _maxVisibleLines) ? linesBuffer.size() - _maxVisibleLines : 0;
    } else {
        firstVisibleLine = lineNumber;
    }
}

String ScrollableTextArea::getLine(size_t lineNumber) {
    return linesBuffer[(lineNumber >= linesBuffer.size()) ? linesBuffer.size() : lineNumber];
}

size_t ScrollableTextArea::getMaxLines() { return linesBuffer.size(); }

void ScrollableTextArea::show(bool force) {
    draw(force);
    while (check(SelPress)) {
        update(force);
        yield();
    }
    while (!check(SelPress)) {
        update(force);
        yield();
    }
}

uint32_t ScrollableTextArea::getMaxVisibleTextLength() { return _maxVisibleLines * 20; }

void ScrollableTextArea::update(bool force) {
    if (check(PrevPress) || check(UpPress)) scrollUp();
    else if (check(NextPress) || check(DownPress)) scrollDown();
    draw(force);
}

void ScrollableTextArea::fromFile(File file) {
    linesBuffer.clear();
    while (file.available()) addLine(file.readStringUntil('\n'));
    draw(true);
    delay(100);
    draw(true);
}

void ScrollableTextArea::clear() {
    firstVisibleLine = 0;
    linesBuffer.clear();
}

void ScrollableTextArea::fromString(const String &text) {
    clear();
    int startIdx = 0;
    int endIdx = 0;
    while (endIdx < text.length()) {
        if (text[endIdx] == '\n') {
            addLine(text.substring(startIdx, endIdx));
            startIdx = endIdx + 1;
        }
        endIdx++;
    }
    if (startIdx < text.length()) { addLine(text.substring(startIdx, endIdx)); }
}
