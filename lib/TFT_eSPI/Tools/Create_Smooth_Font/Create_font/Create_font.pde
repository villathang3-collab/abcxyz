import processing.serial.*;

// =========================================================================
// CẤU HÌNH NGƯỜI DÙNG
// =========================================================================

// Tên file font trong thư mục "data"
String fontFileName = "unifont-16.0.02.otf"; 

// Cấu hình Font
int fontSize = 8;          // Kích thước 8px
boolean isSmooth = false;  // [ĐÃ SỬA TÊN BIẾN] FALSE = Pixel (sắc nét), TRUE = Mượt

// Cấu hình Xuất file
boolean doExport = true;   // True để tạo file .h
String shortName = "Unifont8"; 

// =========================================================================
// KHÔNG CẦN SỬA DƯỚI ĐÂY
// =========================================================================

static final int[] unicodeBlocks = {
  0x0020, 0x007E, // Basic Latin
  0x00C0, 0x00FF, // Latin-1 Supplement
  0x0102, 0x0103, // Ă ă
  0x0110, 0x0111, // Đ đ
  0x0128, 0x0129, // Ĩ ĩ
  0x0168, 0x0169, // Ũ ũ
  0x01A0, 0x01A1, // Ơ ơ
  0x01AF, 0x01B0, // Ư ư
  0x1EA0, 0x1EF9  // Tiếng Việt mở rộng
};

PFont fontSave;

void setup() {
  size(800, 600); 
  
  // [QUAN TRỌNG] Tắt làm mượt toàn cục
  if (!isSmooth) {
    noSmooth(); 
  }
  
  // 1. Tạo charset (Danh sách ký tự cần lấy)
  int count = 0;
  int blockCount = unicodeBlocks.length;
  for (int i = 0; i < blockCount; i+=2) {
    count += (unicodeBlocks[i+1] - unicodeBlocks[i] + 1);
  }
  char[] charset = new char[count];
  int index = 0;
  for (int i = 0; i < blockCount; i+=2) {
    for (int code = unicodeBlocks[i]; code <= unicodeBlocks[i+1]; code++) {
      charset[index++] = Character.toChars(code)[0];
    }
  }

  // 2. Load Font
  println("Dang tai font " + fontFileName + " size " + fontSize + "...");
  // Sử dụng biến isSmooth thay vì smooth
  fontSave = createFont(fontFileName, fontSize, isSmooth, charset);
  
  if (fontSave == null) {
    println("LOI: Khong tim thay file " + fontFileName + " trong thu muc data!");
    exit();
    return;
  }

  // 3. XUẤT FILE 
  if (doExport) {
    exportFontFile(shortName);
  } else {
    println("CHE DO TEST: Khong tao file .h");
  }
}

void draw() {
  background(20); 
  fill(255);      
  
  // Nếu muốn hiển thị pixel rõ ràng trên màn hình Preview, ta tắt smooth lần nữa
  if (!isSmooth) noSmooth();
  
  textFont(fontSave);
  
  // --- PHẦN HIỂN THỊ TEST ---
  int x = 10;
  int y = 20;
  int spacing = fontSize + 8; 
  
  text("PREVIEW FONT: " + fontFileName + " (" + fontSize + "px)", x, y);
  stroke(100); line(x, y+5, width-20, y+5);
  
  y += spacing * 2;
  fill(255, 255, 0); 
  text("1. Test Tiếng Việt:", x, y);
  
  fill(255);
  y += spacing;
  text("Cộng hòa xã hội chủ nghĩa Việt Nam", x, y);
  y += spacing;
  text("á à ả ã ạ ă ắ ằ ẳ ẵ ặ â ấ ầ ẩ ẫ ậ", x, y);
  y += spacing;
  text("đ Đ ư ứ ừ ử ữ ự ơ ớ ờ ở ỡ ợ", x, y);
  
  y += spacing * 2;
  fill(100, 255, 100); 
  text("2. Test ASCII & Số:", x, y);
  fill(255);
  y += spacing;
  text("The quick brown fox jumps over the lazy dog.", x, y);
  y += spacing;
  text("0123456789 !@#$%^&*()", x, y);
  
  y += spacing * 2;
  fill(100, 150, 255);
  text("TRẠNG THÁI:", x, y);
  y += spacing;
  if (doExport) text("ĐÃ XUẤT FILE! Xem thư mục FontFiles/" + shortName + ".h", x, y);
  else text("Đang xem thử (Chưa xuất file)", x, y);

  noLoop();
}

void exportFontFile(String name) {
  String outputName = "FontFiles/" + name;
  
  try {
    // Bước 1: Lưu file .vlw
    OutputStream output = createOutput(outputName + ".vlw");
    fontSave.save(output);
    output.close();
    
    // Bước 2: Chuyển sang Hex (.h)
    InputStream input = createInputRaw(outputName + ".vlw");
    if (input == null) input = createInput(outputName + ".vlw");
    
    PrintWriter outputH = createWriter(outputName + ".h");
       
    outputH.println("#include <pgmspace.h>");
    outputH.println();
    outputH.println("// Font: " + fontFileName + " | Size: " + fontSize + "px | NoSmooth");
    outputH.println("const uint8_t " + name + "[] PROGMEM = {");
       
    int i = 0; 
    int data = input.read();
    while(data != -1){
      if (data < 16) outputH.print("0x0" + hex(data));
      else outputH.print("0x" + hex(data));
      
      data = input.read();
      if (data != -1) outputH.print(", ");
      if(++i >= 16) { outputH.println(); i = 0; }
    }
       
    outputH.println("\n};"); 
    outputH.close(); 
    input.close();
    
    println("THANH CONG! File nam tai: FontFiles/" + name + ".h");
  } catch(Exception e) {
    println("LOI XUAT FILE: " + e.getMessage());
    e.printStackTrace();
  }
}
