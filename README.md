# Soccer Pool - Trò chơi Bóng đá Bida (MVC)

Chào mừng bạn đến với **Soccer Pool**, một trò chơi kết hợp độc đáo giữa môn bóng đá và kỹ thuật sút bida. Trò chơi được xây dựng bằng ngôn ngữ **C++** sử dụng thư viện **SFML 3.0.2** theo mô hình kiến trúc **MVC (Model-View-Controller)**.

## 🌟 Tính năng nổi bật

- **Đa dạng chế độ chơi**:
  - **PvP (Player vs Player)**: So tài cùng bạn bè trên cùng một máy tính.
  - **PvAI (Player vs AI)**: Thử thách kỹ năng với trí tuệ nhân tạo.
  - **AI vs AI**: Xem trận đấu giả lập giữa hai phe máy.
- **20 Đội bóng quốc gia**: Bao gồm Việt Nam, Argentina, Brazil, Pháp, Đức, Anh, Nhật Bản, Hàn Quốc... với logo và màu sắc riêng biệt.
- **Hệ thống sơ đồ chiến thuật**: Cho phép người chơi lựa chọn các đội hình khác nhau như 1-2-2, 1-1-3, 1-2-1-1,... giúp tăng tính chiến thuật.
- **Vật lý chân thực**: Hệ thống vật lý được xây dựng chi tiết cho cảm giác va chạm, nảy tường và ma sát của bóng/cầu thủ.
- **Menu Tiện ích**:
  - **Cài đặt (Options)**: Tùy chỉnh âm lượng nhạc nền và hiệu ứng âm thanh.
  - **Xác nhận thoát**: Tránh việc vô tình thoát trận đấu đang diễn ra.
- **Hiệu ứng & Âm thanh**:
  - Hiệu ứng hoạt ảnh khi ghi bàn (Goal Animation).
  - Âm thanh tiếng còi khai cuộc, tiếng va chạm của bóng và cầu thủ.
  - Giao diện người dùng (UI) hiện đại, hỗ trợ chọn đội và xem bảng tỉ số.
- **Giới hạn thời gian**: Mỗi lượt đi có giới hạn 30 giây, tạo áp lực và kịch tính cho trận đấu.

## 🛠 Yêu cầu phần mềm

Để chạy hoặc phát triển dự án này, bạn cần có:

- **Hệ điều hành**: Windows 10 hoặc Windows 11.
- **IDE**: [Visual Studio 2022](https://visualstudio.microsoft.com/) (Khuyên dùng bộ công cụ Desktop development with C++).
- **Thư viện**: **SFML 3.0.2** (Các file headers, libs và DLLs đã được đính kèm sẵn trong thư mục dự án `SFML_3.0.2`).

## 🚀 Cách cài đặt và chạy ứng dụng

1. **Tải mã nguồn**: Clone hoặc tải file .zip của repository này về máy.
2. **Mở dự án**: Tìm và mở file `SoccerPool.sln` bằng Visual Studio 2022.
3. **Cấu hình Build**:
   - Chọn cấu hình là **Release** (hoặc Debug) và nền tảng là **x64**.
4. **Biên dịch**: Nhấn `Ctrl + Shift + B` để Build Solution.
5. **Chạy trò chơi**: Nhấn `F5` hoặc nút **Start** trong Visual Studio.
   - *Lưu ý*: Hãy đảm bảo các file DLL của SFML và thư mục `assets` nằm cùng cấp với file `.exe` được sinh ra trong thư mục `x64\Release`.

## 🎮 Hướng dẫn sử dụng

### 1. Thao tác điều khiển
- **Sút bóng**: Nhấn **chuột trái** vào cầu thủ của đội mình (khi đến lượt), **kéo ngược lại** về phía sau để lấy lực và hướng, sau đó **thả chuột** để sút.
- **Độ mạnh**: Độ dài của đường kéo càng dài thì lực sút càng mạnh.
- **Luật chơi**: Mỗi đội có 30 giây để thực hiện lượt sút. Nếu bóng hoặc cầu thủ chưa dừng hẳn, bạn chưa thể sút lượt tiếp theo.
- **Chơi lại**: Khi trò chơi kết thúc (GameOver), nhấn phím **Space** để quay lại menu chính.

### 2. Các bước bắt đầu
- Từ màn hình chính, nhấn **PLAY**.
- Chọn chế độ chơi (PvP, PvAI hoặc AI vs AI).
- Chọn đội bóng yêu thích cho Đội 1 và Đội 2.
- Chọn sơ đồ chiến thuật mong muốn.
- Trận đấu sẽ bắt đầu sau tiếng còi của trọng tài!

## 🏗 Kiến trúc dự án

Dự án được tổ chức theo mô hình **MVC**:
- **Model (`GameState`, `Ball`, `Piece`, `PhysicsEngine`)**: Quản lý dữ liệu logic, trạng thái trò chơi và tính toán vật lý.
- **View (`Game_Render`)**: Đảm nhận việc vẽ hình ảnh lên cửa sổ SFML, xử lý hoạt ảnh và âm thanh.
- **Controller (`Game_Controller`)**: Cầu nối xử lý sự kiện từ người dùng (chuột, bàn phím) và điều phối giữa Model và View.

## 🤝 Đóng góp
Nếu bạn có ý tưởng cải tiến trò chơi, hãy liên hệ với nhóm để đóng góp. Mọi sự đóng góp đều được trân trọng!

---
*Chúc bạn có những giây phút giải trí vui vẻ với Soccer Pool!*
