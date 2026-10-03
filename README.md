# HNStudio Musik AI

Project mới: Electron UI + tiến trình native `hn-audio.exe`. Đây là mã nguồn có triển khai audio thật, không phải bản UI giả. **Chưa có kết quả build MSVC Release và chưa kiểm chứng âm thanh trên Windows. Không coi bản này là đã nghiệm thu hoặc bảo đảm không lỗi.**

## Tải EXE qua GitHub Actions

1. Giải nén ZIP. Đưa toàn bộ nội dung bên trong thư mục `HNStudio-Musik-AI` vào gốc repository, gồm cả `.github`. `package.json` phải ở gốc repository.
2. Vào **Actions → Windows Release x64 → Run workflow**. Workflow có sẵn `main.yml` và cũng chạy khi push `main` / `master`.
3. Khi job thành công, tải artifact **HNStudio-Musik-AI-Windows-x64**: gồm portable `HNStudio-Musik-AI-1.0.0-x64.exe` và installer `HNStudio-Musik-AI-Setup-1.0.0-x64.exe`.

Không cần sửa từng file. Workflow cài npm dependencies theo lockfile, tải JUCE 8.0.6 và nlohmann/json 3.11.3, build native Release x64, chạy bài tự kiểm tra DSP độc lập không khởi động backend hoặc thiết bị âm thanh, đóng gói Electron rồi kiểm tra native backend có trong gói trước khi upload EXE. Không cần GitHub token riêng. File EXE chưa ký số.

## Yêu cầu Windows

- Windows x64, khuyến nghị Windows 11. Process loopback yêu cầu Windows build 20348 trở lên. Windows 10 thông thường build 19045 không hỗ trợ đường loopback này; app trả lỗi rõ ràng thay vì dùng fallback gây feedback.
- Quyền microphone cho desktop apps phải được bật.
- Native dùng WASAPI shared mode; driver phải hỗ trợ cấu hình đang chọn. Buffer / sample rate thực tế được engine trả lại cho UI. Độ trễ thực tế phụ thuộc driver và phần cứng; chưa đo ở bản này.
- Không cần VB-CABLE hay Stereo Mix. VST3 là tùy chọn; các DSP tích hợp vẫn hoạt động khi không có plugin.

## Nghiệm thu 3 đường âm thanh trên Windows trước

1. Chọn microphone / output, Apply audio. Bấm **TEST 440 Hz**: tone 2 giây phải phát ở output đã chọn, không phụ thuộc nút LIVE hoặc các volume. Kiểm tra bằng đổi giữa hai output. Tone vẫn đi qua limiter an toàn.
2. Bật LIVE, tắt tất cả DSP và VST3, đặt Music Volume = 0. Nói vào mic: MIC / MASTER meter phải chạy và nghe mic ở output. Ưu tiên tai nghe để tránh hồi tiếp âm học từ loa vào mic.
3. Tắt tiếng mic bằng Mic Volume = 0. Mở YouTube từ Chrome bên ngoài app. MUSIC / MASTER phải chạy; tăng giảm Music Volume để kiểm tra đường mix. App không mở YouTube bên trong giao diện.

**Quan trọng về nhạc hai lần:** process loopback loại trừ `hn-audio.exe`, nên không bắt lại chính MASTER. Tuy nhiên nó không chặn đường phát gốc của Chrome. Nếu Chrome và MASTER cùng ra một loa, loa sẽ nhận cả bản gốc và bản mix có độ trễ. Để nghe đúng MASTER, vào Windows Volume Mixer / App volume and device preferences, chọn output của Chrome khác output app (ví dụ Chrome ra HDMI, app ra tai nghe USB). Bản này không tự chuyển/mute đường phát gốc; với máy chỉ có một endpoint output, không thể bảo đảm nghe một bản nhạc duy nhất theo kiến trúc capture + phát lại này.

Chỉ sau khi ba mục trên đạt mới bật từng DSP, thêm VST3 và nghiệm thu REC. Workflow headless không thay thế thử loa/mic thực tế.

## Chức năng đã có trong mã nguồn

- LIVE ON/OFF; microphone/output; 44.1/48/96 kHz; buffer 128–2048.
- Mic → Gate → Compressor → EQ 13 band → De-Esser → pitch correction → Reverb → bốn VST3 slots → Mic Volume.
- Âm thanh các tiến trình Windows khác → WASAPI process loopback → Music Volume.
- Mic + Music → Master Volume → stereo-linked peak limiter → output; waveform và meters lấy từ tín hiệu native.
- Gate envelope; compressor threshold/ratio; EQ peaking filters; de-esser đơn giản; reverb bốn comb delays.
- Pitch correction cơ bản: autocorrelation + dịch cao độ bằng dual delay / crossfade. Key C, Cm, D, Dm, E, Em, F, Fm; Auto Key dùng chroma histogram từ giọng mic và có thể ước lượng cả 12 trưởng / 12 thứ. Đây không phải sản phẩm Auto-Tune thương mại, chưa được đánh giá chất lượng thực tế; có thể có artifact, sai octave hoặc sai key. Auto Key từ một giọng đơn cần nhiều nốt và có thể mơ hồ giữa trưởng/thứ. Key thủ công giới hạn theo yêu cầu.
- VST3 Load / ON-OFF / Bypass / Remove; stereo input/output; generic parameter sliders. Không có cửa sổ editor riêng của plugin trong bản này. Trạng thái binary và đường dẫn VST được lưu trong project. VST chạy trong tiến trình native, không sandbox từng plugin; plugin lỗi có thể làm backend dừng và UI báo lỗi.
- REC Master stereo WAV PCM 24-bit qua disk writer thread. Counter báo khi queue ghi bị đầy. STOP REC / thoát app bình thường flush dữ liệu.
- Save/Load `.hnproj`: thiết bị, cấu hình, volume, DSP, key, EQ và VST states. Không tự bật LIVE/REC khi load. VST phải có sẵn ở đường dẫn đã lưu trên máy đó.
- Lỗi thiết bị, loopback HRESULT, backend exit, WAV queue overflow và oversized callback được báo ở audio log. Có Export log.
- UI dark neon gradient, panel kính, viền chạy khi ON, LIVE pulse, 13 faders và meters. UI animation tách khỏi audio process.

Thay đổi DSP / plugin và Save Project tạm dừng callback khi cập nhật để tránh data race; thao tác đó có thể gây một khoảng ngắt ngắn. Không hứa realtime parameter automation không gián đoạn. Hai clock capture/output được đệm và giới hạn backlog; chưa có asynchronous drift resampler nên phiên LIVE dài cần được kiểm tra drop / gián đoạn.

## Kiến trúc

`electron/main.cjs` quản lý backend và các hộp thoại file; `preload.cjs` chỉ expose API giới hạn, renderer không có Node. IPC Electron đi tới main, sau đó JSON Lines qua stdin/stdout đến native; không truyền audio qua IPC. Command xử lý trên JUCE message thread; mic/output callback chạy riêng, loopback trên capture thread, ghi WAV trên disk thread. Telemetry 25 Hz. Tín hiệu audio không phụ thuộc tốc độ renderer.

`native/engine.cpp`: device lifecycle, mixer, limiter, VST3, recorder, command/telemetry.
`native/loopback.h`: WASAPI asynchronous activation, process exclusion và SPSC stereo queue.
`native/dsp.h`: vocal DSP với bộ nhớ cấp phát trước.

Loopback async capture không cần WasapiDriver/AudioBusRouter. Output và microphone do JUCE WASAPI quản lý. Plugin loading / project loading chạy trên message thread, không chạy trong audio callback. VST3 là third-party code nên không thể bảo đảm plugin không cấp phát trong processBlock.

## Build trên máy Windows

Cần Node.js 22, Visual Studio 2022 Desktop development with C++, CMake 3.24+, Windows SDK hiện đại (Windows 11 SDK). Lệnh đầy đủ:

```powershell
npm ci
npm run build:native
npm run check
.\build\native\Release\hn-dsp-selftest.exe
npm run dist
```

`npm start` dùng backend Release đã build. Electron dev/installer/portable đều dùng cùng native EXE, không có mock audio mode.

## Kết quả kiểm tra khi tạo project

- PASS: JavaScript syntax cho main/preload/renderer; kiểm tra ID UI và cấu hình đóng gói native.
- PASS: `dsp_test.cpp` biên dịch bằng g++ C++20; workflow Windows biên dịch và chạy bài test native độc lập; kiểm tra tín hiệu hữu hạn / khác zero với EQ, pitch correction, Auto Key, reverb tại 44.1/48/96 kHz; silence qua gate.
- PASS: đối chiếu API native với mã nguồn JUCE 8.0.6 đã tải.
- CHƯA KIỂM CHỨNG: native Windows compilation/linking; electron-builder Windows packaging; Windows audio hardware; chất lượng DSP, mọi VST, độ trễ và capture clock dài hạn.
- CHƯA KIỂM CHỨNG: render UI trong Chromium; runtime này không có browser executable và bản tải browser không hợp lệ.

Không có file EXE đã build trong ZIP. Không có thông tin đăng nhập/quyền chạy GitHub Actions của repository trong phiên tạo project này. Không thể báo “Release thành công” khi chưa có log Windows thành công.

## License

Mã nguồn project: AGPL-3.0-only. JUCE 8.0.6 được dùng theo lựa chọn AGPLv3. Nếu muốn phân phối ứng dụng đóng mã nguồn, cần xem xét giấy phép thương mại JUCE và giấy phép các dependency/plugin. Pitch correction trong project không sử dụng thương hiệu hoặc code Antares.
