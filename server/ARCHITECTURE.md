# RtspProject: 라이브 H.264 스트리밍 시스템 아키텍처

이 문서는 `RtspProject`의 전체 아키텍처, 구성 요소의 역할, 그리고 데이터 흐름에 대해 설명합니다. 이 프로젝트는 V4L2 호환 카메라(예: 라즈베리 파이 카메라)로부터 H.264 비디오 스트림을 받아, 표준 RTSP/RTP 프로토콜을 통해 VLC와 같은 미디어 플레이어로 실시간 전송하는 것을 목표로 합니다.

## 1. 핵심 아키텍처

이 시스템은 크게 2개의 애플리케이션으로 구성됩니다.

1.  **카메라 클라이언트 (`camera_sender`):** 카메라 하드웨어에서 직접 영상을 인코딩하고, 이를 NAL 유닛 단위로 분해하여 서버로 전송하는 역할. (주로 라즈베리 파이 같은 임베디드 장치에서 실행)
2.  **RTSP 서버 (`rtsp_server`):** `camera_sender`로부터 H.264 데이터를 수신하고, VLC 같은 표준 RTSP 클라이언트의 요청에 따라 이 데이터를 RTSP/RTP 프로토콜로 스트리밍하는 역할. (PC 또는 다른 서버에서 실행)



## 2. 구성 요소별 역할 및 핵심 로직

### 2.1. 카메라 클라이언트 (`camera_sender`)

-   **역할:** V4L2 카메라에서 H.264 Annex B 스트림을 캡처하고, 이를 NAL 유닛 단위로 파싱하여 서버의 `CameraReceiver`에 TCP로 전송합니다.
-   **핵심 로직:**
    1.  **V4L2 캡처 (`V4L2Capture`):** H.264로 인코딩된 원본 비디오 데이터 덩어리(버퍼)를 카메라 드라이버로부터 가져옵니다. 이때, IDR 프레임 앞에 SPS/PPS가 포함되도록 드라이버에 `V4L2_CID_MPEG_VIDEO_H264_SPS_PPS_TO_IDR` 컨트롤을 설정합니다.
    2.  **상태 기반 파서 (`main.cpp`의 `processData`):**
        -   `grabFrame`으로 받은 데이터 덩어리는 하나의 NAL 유닛보다 크거나 작을 수 있으며, 여러 NAL 유닛을 포함하거나 NAL 유닛의 일부만 포함할 수 있습니다.
        -   이를 처리하기 위해, 받은 데이터를 `g_processBuffer`라는 전역 버퍼에 계속 축적합니다.
        -   이 버퍼 안에서 `0x000001` 또는 `0x00000001` 형태의 'Start Code'를 찾아 완전한 NAL 유닛(다음 Start Code가 발견된)만을 잘라냅니다.
        -   버퍼 끝에 있는 불완전한 NAL 유닛 조각은 다음 데이터를 기다리기 위해 버퍼에 남겨둡니다.
    3.  **TCP 전송 (`TcpClient`):**
        -   잘라낸 각 NAL 유닛(Start Code 포함)의 앞에 4바이트 길이 정보를 붙여서, 서버의 **8556 포트**로 전송합니다.

### 2.2. RTSP 서버 (`rtsp_server`)

#### `CameraReceiver`
-   **역할:** **8556 포트**에서 `camera_sender`의 연결을 기다리고, NAL 유닛 데이터를 수신하여 공유 버퍼인 `StreamBuffer`에 넣습니다.
-   **핵심 로직:**
    1.  클라이언트로부터 `[4바이트 길이] + [NAL 유닛 데이터]` 형식의 메시지를 수신합니다.
    2.  수신한 NAL 유닛 데이터 안에서 Start Code를 건너뛴 위치의 바이트를 읽어, NAL 유닛의 실제 타입(SPS=7, PPS=8, IDR=5 등)을 정확히 식별합니다. (주요 버그 수정 지점)
    3.  타입이 7 또는 8인 경우, 해당 NAL 유닛을 `StreamBuffer`의 SPS/PPS 저장 공간에 저장합니다.
    4.  모든 NAL 유닛은 `RtpSender`가 사용할 수 있도록 `StreamBuffer`의 메인 큐에 `push`합니다.

#### `StreamBuffer`
-   **역할:** 생산자-소비자 패턴을 위한 스레드 안전 큐. `CameraReceiver`가 생산자, `RtpSender`가 소비자 역할을 합니다. 또한, 전체 세션에서 사용할 SPS/PPS 정보를 보관하는 저장소 역할도 겸합니다.

#### `TcpServer` & `RtspSession`
-   **역할:** **8554 포트**에서 VLC와 같은 표준 RTSP 클라이언트의 연결을 받고, RTSP 시그널링(OPTIONS, DESCRIBE, SETUP, PLAY 등)을 처리합니다.
-   **핵심 로직 (`RtspSession`):**
    1.  **`DESCRIBE` 처리:**
        -   클라이언트로부터 `DESCRIBE` 요청을 받으면, `StreamBuffer`에 SPS/PPS가 저장될 때까지 기다립니다.
        -   저장된 SPS/PPS NAL 유닛에서 Start Code를 제거하고, 순수 데이터만 Base64로 인코딩합니다.
        -   인코딩된 `sprop-parameter-sets` 정보를 포함한 유효한 SDP(Session Description Protocol)를 생성하여 클라이언트에 응답합니다.
    2.  **`SETUP` 처리:** 클라이언트가 RTP 패킷을 받을 UDP 포트 정보를 설정하고, `RtpSender`를 초기화합니다.
    3.  **`PLAY` 처리:** `RtpSender`의 스트리밍 스레드를 시작시킵니다.

#### `RtpSender`
-   **역할:** `StreamBuffer`에서 NAL 유닛을 꺼내와, RTP 패킷으로 조립하여 클라이언트의 UDP 포트로 전송합니다.
-   **핵심 로직:**
    1.  `StreamBuffer`에서 NAL 유닛(Start Code 포함)을 `pop`합니다.
    2.  NAL 유닛에서 Start Code를 제거하여 순수 NAL 데이터만 얻습니다.
    3.  NAL 데이터 크기에 따라 RTP 패킷을 조립합니다.
        -   **작은 NAL 유닛:** 하나의 RTP 패킷에 담아 전송합니다.
        -   **큰 NAL 유닛:** H.264 분할 표준인 `FU-A` 모드에 따라 여러 개의 RTP 패킷으로 쪼개서 전송합니다.
    4.  **타임스탬프 및 마커 비트 처리:**
        -   실제 영상 데이터(VCL NAL, 타입 1~5)일 경우에만 타임스탬프를 증가시킵니다. (SPS/PPS는 타임스탬프에 영향을 주지 않음)
        -   프레임의 마지막을 의미하는 마커 비트(Marker Bit)를 설정하여, 디코더가 프레임 경계를 인식하도록 돕습니다. (현재는 '영상 NAL 유닛 하나 = 한 프레임'으로 단순화하여 처리)

## 3. 총 정리: 데이터 흐름

1.  **`camera_sender`**가 V4L2 드라이버로부터 H.264 버퍼를 받습니다.
2.  이 버퍼는 Start Code 기준으로 NAL 유닛들로 파싱됩니다.
3.  파싱된 각 NAL 유닛은 `[길이][Start Code][NAL 데이터]` 형태로 TCP를 통해 **서버 8556 포트**로 전송됩니다.
4.  **`CameraReceiver`**가 이 메시지를 받아 NAL 유닛 타입을 정확히 식별하고, `StreamBuffer`에 저장합니다.
5.  **`VLC`**가 **서버 8554 포트**로 접속하여 `DESCRIBE`를 요청합니다.
6.  **`RtspSession`**은 `StreamBuffer`의 SPS/PPS로 SDP를 만들어 `VLC`에 응답합니다.
7.  **`VLC`**는 `SETUP`, `PLAY`를 요청합니다.
8.  **`RtpSender`**는 `StreamBuffer`에서 NAL 유닛을 꺼내 Start Code를 제거하고, RTP 패킷으로 조립합니다.
9.  RTP 패킷이 `VLC`의 UDP 포트로 전송됩니다.
10. **`VLC`**가 RTP 패킷을 받아 영상을 디코딩하고 화면에 표시합니다.
