# Qt ↔ Unity 통합 가이드

MediaPipeUnityPlugin을 사용하여 Qt에서 Unity로 blendshape 데이터를 전달하고 아바타를 렌더링하는 방법을 설명합니다.

## 아키텍처 개요

```
Qt Client (C++)
  ├─ MediaPipe (Python) → Blendshape 데이터 (52개)
  │   └─ FaceData::Blendshape { category: QString, score: float }
  │
  ├─ Unity 프로세스 (별도 프로세스)
  │   ├─ MediaPipeUnityPlugin → FaceLandmarkerResult
  │   │   └─ faceBlendshapes: List<Classifications>
  │   │       └─ categories: List<Category> { categoryName: string, score: float }
  │   │
  │   ├─ Blendshape 매핑 (52개 모두 지원)
  │   │   └─ SkinnedMeshRenderer.SetBlendShapeWeight(index, weight)
  │   │
  │   └─ RenderTexture → 이미지 스트림
  │
  └─ Qt에서 Unity 렌더링 이미지를 QML에 표시
```

## 1. 데이터 형식 비교

### Qt (MediaPipeProcessor)
```cpp
struct Blendshape {
    QString category;  // 예: "eyeBlinkLeft", "mouthSmileLeft"
    float score;       // 0.0 ~ 1.0
};
```

### Unity (MediaPipeUnityPlugin)
```csharp
// FaceLandmarkerResult.faceBlendshapes[0] = Classifications
// Classifications.categories = List<Category>
// Category.categoryName = string
// Category.score = float
```

**매핑**: 두 형식은 동일합니다. `category` ↔ `categoryName`, `score` ↔ `score`

## 2. Unity 프로젝트 설정

### 2.1 MediaPipeUnityPlugin 설치

1. [MediaPipeUnityPlugin 릴리스 페이지](https://github.com/homuler/MediaPipeUnityPlugin/releases)에서 패키지 다운로드
2. Unity 프로젝트에 `.unitypackage` 또는 `.tgz` 파일 임포트
3. `Assets/MediaPipeUnity/Samples/Scenes/Face Landmark Detection` 샘플 씬 확인

### 2.2 프로젝트 구조

```
unity_avatar/
├── Assets/
│   ├── Scripts/
│   │   ├── BlendshapeReceiver.cs          # Qt로부터 blendshape 데이터 수신
│   │   ├── AvatarBlendshapeController.cs   # Unity 아바타에 blendshape 적용
│   │   └── RenderTextureSender.cs          # 렌더링 결과를 Qt로 전송
│   ├── Models/
│   │   └── Avatar02.glb                    # GLB 아바타 모델
│   └── Scenes/
│       └── AvatarScene.unity                # 메인 씬
└── ProjectSettings/
```

## 3. Unity 스크립트 구현

### 3.1 BlendshapeReceiver.cs - Qt로부터 데이터 수신

```csharp
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;
using Newtonsoft.Json; // JSON.NET 또는 Unity JsonUtility 사용

public class BlendshapeReceiver : MonoBehaviour
{
    [Header("Network Settings")]
    public int listenPort = 12345;
    
    [Header("Debug")]
    public bool logReceivedData = false;
    
    private TcpListener tcpListener;
    private TcpClient tcpClient;
    private NetworkStream stream;
    private bool isListening = false;
    
    // Qt로부터 받은 blendshape 데이터
    public class BlendshapeData
    {
        public List<BlendshapeItem> blendshapes;
    }
    
    public class BlendshapeItem
    {
        public string category;
        public float score;
    }
    
    // 이벤트: blendshape 데이터가 수신되었을 때
    public event Action<List<BlendshapeItem>> OnBlendshapeReceived;
    
    void Start()
    {
        StartListening();
    }
    
    void OnDestroy()
    {
        StopListening();
    }
    
    public void StartListening()
    {
        if (isListening) return;
        
        try
        {
            tcpListener = new TcpListener(IPAddress.Any, listenPort);
            tcpListener.Start();
            isListening = true;
            
            Debug.Log($"[BlendshapeReceiver] Listening on port {listenPort}");
            
            // 비동기로 클라이언트 연결 대기
            tcpListener.BeginAcceptTcpClient(OnClientConnected, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"[BlendshapeReceiver] Failed to start listening: {e.Message}");
        }
    }
    
    public void StopListening()
    {
        isListening = false;
        
        stream?.Close();
        tcpClient?.Close();
        tcpListener?.Stop();
        
        Debug.Log("[BlendshapeReceiver] Stopped listening");
    }
    
    private void OnClientConnected(IAsyncResult ar)
    {
        try
        {
            tcpClient = tcpListener.EndAcceptTcpClient(ar);
            stream = tcpClient.GetStream();
            
            Debug.Log("[BlendshapeReceiver] Client connected");
            
            // 다음 클라이언트를 위해 다시 대기
            if (isListening)
            {
                tcpListener.BeginAcceptTcpClient(OnClientConnected, null);
            }
            
            // 데이터 수신 시작
            StartReceiving();
        }
        catch (Exception e)
        {
            Debug.LogError($"[BlendshapeReceiver] Error accepting client: {e.Message}");
        }
    }
    
    private void StartReceiving()
    {
        byte[] buffer = new byte[4096];
        StringBuilder messageBuilder = new StringBuilder();
        
        while (tcpClient != null && tcpClient.Connected)
        {
            try
            {
                int bytesRead = stream.Read(buffer, 0, buffer.Length);
                if (bytesRead == 0)
                {
                    // 연결 종료
                    break;
                }
                
                string data = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                messageBuilder.Append(data);
                
                // JSON 메시지가 완성되었는지 확인 (간단한 방법: '\n' 또는 '}' 개수)
                string fullMessage = messageBuilder.ToString();
                if (fullMessage.Contains("}") && fullMessage.Trim().EndsWith("}"))
                {
                    ProcessMessage(fullMessage.Trim());
                    messageBuilder.Clear();
                }
            }
            catch (Exception e)
            {
                Debug.LogError($"[BlendshapeReceiver] Error receiving data: {e.Message}");
                break;
            }
        }
        
        // 연결 종료 처리
        stream?.Close();
        tcpClient?.Close();
        Debug.Log("[BlendshapeReceiver] Client disconnected");
    }
    
    private void ProcessMessage(string jsonMessage)
    {
        try
        {
            if (logReceivedData)
            {
                Debug.Log($"[BlendshapeReceiver] Received: {jsonMessage}");
            }
            
            // JSON 파싱
            BlendshapeData data = JsonConvert.DeserializeObject<BlendshapeData>(jsonMessage);
            // 또는 Unity JsonUtility 사용:
            // BlendshapeData data = JsonUtility.FromJson<BlendshapeData>(jsonMessage);
            
            if (data != null && data.blendshapes != null)
            {
                OnBlendshapeReceived?.Invoke(data.blendshapes);
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"[BlendshapeReceiver] Error parsing JSON: {e.Message}");
        }
    }
}
```

### 3.2 AvatarBlendshapeController.cs - 아바타에 blendshape 적용

```csharp
using System.Collections.Generic;
using UnityEngine;

public class AvatarBlendshapeController : MonoBehaviour
{
    [Header("Avatar Settings")]
    public SkinnedMeshRenderer avatarMeshRenderer;
    
    [Header("Blendshape Mapping")]
    public bool useMediaPipeOrder = true; // MediaPipe 순서 사용 여부
    
    // MediaPipe blendshape 이름 → Unity BlendShape 인덱스 매핑
    private Dictionary<string, int> blendshapeMap = new Dictionary<string, int>();
    
    // MediaPipe blendshape 이름 목록 (표준 순서)
    private static readonly string[] MediaPipeBlendshapeNames = new string[]
    {
        "eyeBlinkLeft", "eyeBlinkRight",
        "eyeLookDownLeft", "eyeLookDownRight",
        "eyeLookInLeft", "eyeLookInRight",
        "eyeLookOutLeft", "eyeLookOutRight",
        "eyeLookUpLeft", "eyeLookUpRight",
        "eyeSquintLeft", "eyeSquintRight",
        "eyeWideLeft", "eyeWideRight",
        "browDownLeft", "browDownRight",
        "browInnerUp", "browOuterUpLeft", "browOuterUpRight",
        "noseSneerLeft", "noseSneerRight",
        "cheekPuff", "cheekSquintLeft", "cheekSquintRight",
        "jawForward", "jawLeft", "jawOpen", "jawRight",
        "mouthClose", "mouthDimpleLeft", "mouthDimpleRight",
        "mouthFrownLeft", "mouthFrownRight",
        "mouthFunnel", "mouthLeft", "mouthLowerDownLeft", "mouthLowerDownRight",
        "mouthPressLeft", "mouthPressRight", "mouthPucker", "mouthRight",
        "mouthRollLower", "mouthRollUpper", "mouthShrugLower", "mouthShrugUpper",
        "mouthSmileLeft", "mouthSmileRight",
        "mouthStretchLeft", "mouthStretchRight",
        "mouthUpperUpLeft", "mouthUpperUpRight",
        "tongueOut"
    };
    
    void Start()
    {
        if (avatarMeshRenderer == null)
        {
            avatarMeshRenderer = GetComponent<SkinnedMeshRenderer>();
        }
        
        if (avatarMeshRenderer == null)
        {
            Debug.LogError("[AvatarBlendshapeController] SkinnedMeshRenderer not found!");
            return;
        }
        
        BuildBlendshapeMap();
    }
    
    private void BuildBlendshapeMap()
    {
        Mesh mesh = avatarMeshRenderer.sharedMesh;
        if (mesh == null)
        {
            Debug.LogError("[AvatarBlendshapeController] Mesh not found!");
            return;
        }
        
        int blendShapeCount = mesh.blendShapeCount;
        Debug.Log($"[AvatarBlendshapeController] Found {blendShapeCount} blend shapes in mesh");
        
        // Unity의 BlendShape 이름을 모두 출력 (디버깅용)
        for (int i = 0; i < blendShapeCount; i++)
        {
            string blendShapeName = mesh.GetBlendShapeName(i);
            Debug.Log($"[AvatarBlendshapeController] BlendShape[{i}]: {blendShapeName}");
        }
        
        // MediaPipe 이름 → Unity 인덱스 매핑 생성
        for (int i = 0; i < MediaPipeBlendshapeNames.Length; i++)
        {
            string mediaPipeName = MediaPipeBlendshapeNames[i];
            
            // Unity에서 동일한 이름의 BlendShape 찾기
            for (int j = 0; j < blendShapeCount; j++)
            {
                string unityName = mesh.GetBlendShapeName(j);
                
                // 정확히 일치하거나, 대소문자 무시하고 비교
                if (string.Equals(unityName, mediaPipeName, System.StringComparison.OrdinalIgnoreCase))
                {
                    blendshapeMap[mediaPipeName] = j;
                    Debug.Log($"[AvatarBlendshapeController] Mapped '{mediaPipeName}' → BlendShape[{j}]");
                    break;
                }
            }
        }
        
        Debug.Log($"[AvatarBlendshapeController] Mapped {blendshapeMap.Count} blendshapes");
    }
    
    public void ApplyBlendshapes(List<BlendshapeReceiver.BlendshapeItem> blendshapes)
    {
        if (avatarMeshRenderer == null || avatarMeshRenderer.sharedMesh == null)
        {
            return;
        }
        
        // 먼저 모든 BlendShape를 0으로 리셋
        Mesh mesh = avatarMeshRenderer.sharedMesh;
        for (int i = 0; i < mesh.blendShapeCount; i++)
        {
            avatarMeshRenderer.SetBlendShapeWeight(i, 0f);
        }
        
        // MediaPipe blendshape 적용
        foreach (var item in blendshapes)
        {
            if (blendshapeMap.TryGetValue(item.category, out int blendShapeIndex))
            {
                // score를 0~100 범위로 변환 (MediaPipe는 0~1, Unity는 0~100)
                float weight = Mathf.Clamp01(item.score) * 100f;
                avatarMeshRenderer.SetBlendShapeWeight(blendShapeIndex, weight);
            }
        }
    }
}
```

### 3.3 RenderTextureSender.cs - 렌더링 결과를 Qt로 전송

```csharp
using System;
using System.Collections;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;

public class RenderTextureSender : MonoBehaviour
{
    [Header("Render Settings")]
    public Camera renderCamera;
    public int textureWidth = 640;
    public int textureHeight = 480;
    
    [Header("Network Settings")]
    public string qtHost = "127.0.0.1";
    public int qtPort = 12346;
    public int sendFPS = 30;
    
    [Header("Format")]
    public bool sendAsPNG = true; // PNG 또는 JPEG
    
    private RenderTexture renderTexture;
    private Texture2D texture2D;
    private TcpClient tcpClient;
    private NetworkStream stream;
    private bool isConnected = false;
    
    private float sendInterval;
    private float lastSendTime = 0f;
    
    void Start()
    {
        sendInterval = 1f / sendFPS;
        
        // RenderTexture 생성
        renderTexture = new RenderTexture(textureWidth, textureHeight, 24);
        texture2D = new Texture2D(textureWidth, textureHeight, TextureFormat.RGB24, false);
        
        // 카메라에 RenderTexture 할당
        if (renderCamera == null)
        {
            renderCamera = Camera.main;
        }
        
        if (renderCamera != null)
        {
            renderCamera.targetTexture = renderTexture;
        }
        
        // Qt 클라이언트 연결
        ConnectToQt();
    }
    
    void OnDestroy()
    {
        DisconnectFromQt();
        
        if (renderTexture != null)
        {
            renderTexture.Release();
            Destroy(renderTexture);
        }
        
        if (texture2D != null)
        {
            Destroy(texture2D);
        }
    }
    
    void Update()
    {
        if (isConnected && Time.time - lastSendTime >= sendInterval)
        {
            SendFrame();
            lastSendTime = Time.time;
        }
    }
    
    private void ConnectToQt()
    {
        try
        {
            tcpClient = new TcpClient();
            tcpClient.Connect(qtHost, qtPort);
            stream = tcpClient.GetStream();
            isConnected = true;
            
            Debug.Log($"[RenderTextureSender] Connected to Qt at {qtHost}:{qtPort}");
        }
        catch (Exception e)
        {
            Debug.LogError($"[RenderTextureSender] Failed to connect to Qt: {e.Message}");
            isConnected = false;
        }
    }
    
    private void DisconnectFromQt()
    {
        isConnected = false;
        stream?.Close();
        tcpClient?.Close();
    }
    
    private void SendFrame()
    {
        if (!isConnected || renderTexture == null)
        {
            return;
        }
        
        try
        {
            // RenderTexture에서 픽셀 읽기
            RenderTexture.active = renderTexture;
            texture2D.ReadPixels(new Rect(0, 0, textureWidth, textureHeight), 0, 0);
            texture2D.Apply();
            RenderTexture.active = null;
            
            // 이미지를 바이트 배열로 변환
            byte[] imageData;
            if (sendAsPNG)
            {
                imageData = texture2D.EncodeToPNG();
            }
            else
            {
                imageData = texture2D.EncodeToJPG();
            }
            
            // 헤더: 이미지 크기 (4바이트)
            byte[] header = BitConverter.GetBytes(imageData.Length);
            
            // 데이터 전송
            stream.Write(header, 0, header.Length);
            stream.Write(imageData, 0, imageData.Length);
            stream.Flush();
        }
        catch (Exception e)
        {
            Debug.LogError($"[RenderTextureSender] Error sending frame: {e.Message}");
            DisconnectFromQt();
        }
    }
}
```

## 4. Unity 씬 설정

### 4.1 씬 구성

1. **카메라 설정**
   - `Main Camera`를 아바타를 렌더링할 위치에 배치
   - `RenderTextureSender` 컴포넌트 추가

2. **아바타 모델**
   - GLB 모델을 씬에 추가
   - `AvatarBlendshapeController` 컴포넌트 추가
   - `SkinnedMeshRenderer` 참조 설정

3. **네트워크 리시버**
   - 빈 GameObject 생성 (`BlendshapeReceiver`)
   - `BlendshapeReceiver` 컴포넌트 추가
   - `OnBlendshapeReceived` 이벤트에 `AvatarBlendshapeController.ApplyBlendshapes` 연결

### 4.2 이벤트 연결

```csharp
// BlendshapeReceiver의 Start() 또는 별도 초기화 스크립트에서
void Start()
{
    BlendshapeReceiver receiver = FindObjectOfType<BlendshapeReceiver>();
    AvatarBlendshapeController controller = FindObjectOfType<AvatarBlendshapeController>();
    
    if (receiver != null && controller != null)
    {
        receiver.OnBlendshapeReceived += controller.ApplyBlendshapes;
    }
}
```

## 5. Qt 클라이언트 수정

### 5.1 MainWindow.h에 Unity 통신 멤버 추가

```cpp
// MainWindow.h
private:
    QProcess *unityProcess;
    QTcpSocket *unityBlendshapeSocket;  // Unity로 blendshape 전송
    QTcpServer *unityImageServer;       // Unity로부터 이미지 수신
    QTcpSocket *unityImageSocket;
    
private slots:
    void startUnityProcess();
    void stopUnityProcess();
    void sendBlendshapeDataToUnity(const QVector<MediaPipeProcessor::FaceData> &faces);
    void onUnityImageReceived();
```

### 5.2 MainWindow.cpp 구현

```cpp
// MainWindow.cpp

#include <QProcess>
#include <QTcpSocket>
#include <QTcpServer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QBuffer>
#include <QImageReader>

void MainWindow::startUnityProcess()
{
    if (unityProcess && unityProcess->state() == QProcess::Running) {
        return; // 이미 실행 중
    }
    
    // Unity 실행 파일 경로
    QString unityAppPath = "/path/to/unity_avatar/Build/UnityAvatar.app/Contents/MacOS/UnityAvatar";
    
    unityProcess = new QProcess(this);
    connect(unityProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::stopUnityProcess);
    
    unityProcess->start(unityAppPath);
    
    // Unity가 시작될 때까지 대기 (선택사항)
    if (!unityProcess->waitForStarted(5000)) {
        qDebug() << "Failed to start Unity process";
        return;
    }
    
    // Unity로 blendshape 데이터 전송 소켓 연결
    unityBlendshapeSocket = new QTcpSocket(this);
    connect(unityBlendshapeSocket, &QTcpSocket::connected, []() {
        qDebug() << "Connected to Unity (blendshape)";
    });
    unityBlendshapeSocket->connectToHost("127.0.0.1", 12345);
    
    // Unity로부터 이미지 수신 서버 시작
    unityImageServer = new QTcpServer(this);
    connect(unityImageServer, &QTcpServer::newConnection, this, [this]() {
        unityImageSocket = unityImageServer->nextPendingConnection();
        connect(unityImageSocket, &QTcpSocket::readyRead, this, &MainWindow::onUnityImageReceived);
        qDebug() << "Unity connected (image receiver)";
    });
    
    if (!unityImageServer->listen(QHostAddress::Any, 12346)) {
        qDebug() << "Failed to start image server";
    }
}

void MainWindow::stopUnityProcess()
{
    if (unityBlendshapeSocket) {
        unityBlendshapeSocket->disconnectFromHost();
        unityBlendshapeSocket->deleteLater();
        unityBlendshapeSocket = nullptr;
    }
    
    if (unityImageSocket) {
        unityImageSocket->disconnectFromHost();
        unityImageSocket->deleteLater();
        unityImageSocket = nullptr;
    }
    
    if (unityImageServer) {
        unityImageServer->close();
    }
    
    if (unityProcess) {
        unityProcess->terminate();
        if (!unityProcess->waitForFinished(3000)) {
            unityProcess->kill();
        }
        unityProcess->deleteLater();
        unityProcess = nullptr;
    }
}

void MainWindow::sendBlendshapeDataToUnity(const QVector<MediaPipeProcessor::FaceData> &faces)
{
    if (!unityBlendshapeSocket || unityBlendshapeSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    if (faces.isEmpty()) {
        return;
    }
    
    // 첫 번째 얼굴의 blendshape 데이터 사용
    const auto &face = faces[0];
    
    // JSON 생성
    QJsonObject root;
    QJsonArray blendshapesArray;
    
    for (const auto &bs : face.blendshapes) {
        QJsonObject bsObj;
        bsObj["category"] = bs.category;
        bsObj["score"] = bs.score;
        blendshapesArray.append(bsObj);
    }
    
    root["blendshapes"] = blendshapesArray;
    
    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // JSON 데이터 전송 (줄바꿈 추가하여 메시지 구분)
    unityBlendshapeSocket->write(jsonData + "\n");
}

void MainWindow::onUnityImageReceived()
{
    if (!unityImageSocket) {
        return;
    }
    
    // 헤더 읽기 (4바이트: 이미지 크기)
    if (unityImageSocket->bytesAvailable() < 4) {
        return;
    }
    
    QByteArray header = unityImageSocket->read(4);
    if (header.size() != 4) {
        return;
    }
    
    uint32_t imageSize = 0;
    memcpy(&imageSize, header.data(), 4);
    
    // 이미지 데이터 읽기
    while (unityImageSocket->bytesAvailable() < imageSize) {
        if (!unityImageSocket->waitForReadyRead(100)) {
            return;
        }
    }
    
    QByteArray imageData = unityImageSocket->read(imageSize);
    if (imageData.size() != static_cast<int>(imageSize)) {
        return;
    }
    
    // QImage로 변환
    QImage image;
    if (image.loadFromData(imageData, "PNG")) {
        // QML에 표시 (VideoImageProvider 또는 별도 Image 소스 사용)
        // 예: videoQuick3DWidget->setUnityImage(image);
        qDebug() << "Received Unity image:" << image.size();
    }
}

// MediaPipeProcessor의 faceDetected 시그널 연결
void MainWindow::onFaceDetected(const QVector<MediaPipeProcessor::FaceData> &faces)
{
    // Unity로 blendshape 데이터 전송
    sendBlendshapeDataToUnity(faces);
    
    // 기존 로직 (디버그 출력 등)
    // ...
}
```

## 6. 빌드 및 실행

### 6.1 Unity 빌드

1. Unity Editor에서 `File > Build Settings`
2. Platform: `macOS`
3. `Build` 클릭하여 실행 파일 생성

### 6.2 Qt 클라이언트 실행

1. Qt 애플리케이션 실행
2. 웹캠 모드 활성화
3. `startUnityProcess()` 호출하여 Unity 시작
4. MediaPipe가 얼굴을 탐지하면 자동으로 Unity에 blendshape 전송
5. Unity가 렌더링한 이미지를 Qt에서 수신하여 표시

## 7. MediaPipeUnityPlugin 직접 사용 (선택사항)

만약 Unity 내에서 직접 MediaPipe를 실행하고 싶다면:

```csharp
using Mediapipe.Tasks.Vision.FaceLandmarker;
using Mediapipe.Tasks.Vision.Core;

public class MediaPipeRunner : MonoBehaviour
{
    private FaceLandmarker faceLandmarker;
    
    void Start()
    {
        var options = new FaceLandmarkerOptions(
            baseOptions: new BaseOptions(modelAssetPath: "face_landmarker_v2_with_blendshapes.bytes"),
            runningMode: RunningMode.LIVE_STREAM,
            numFaces: 1,
            outputFaceBlendshapes: true,
            resultCallback: OnFaceLandmarkerResult
        );
        
        faceLandmarker = FaceLandmarker.CreateFromOptions(options);
    }
    
    private void OnFaceLandmarkerResult(FaceLandmarkerResult result, Image image, long timestamp)
    {
        if (result.faceBlendshapes != null && result.faceBlendshapes.Count > 0)
        {
            var blendshapes = result.faceBlendshapes[0];
            // blendshapes.categories를 사용하여 아바타에 적용
        }
    }
}
```

하지만 현재 아키텍처에서는 Qt에서 이미 MediaPipe를 실행하고 있으므로, Unity는 단순히 데이터를 받아서 렌더링만 하면 됩니다.

## 8. 트러블슈팅

### 문제: Unity가 blendshape 데이터를 받지 못함
- **해결**: Qt의 `sendBlendshapeDataToUnity`에서 JSON 형식 확인
- **해결**: Unity의 `BlendshapeReceiver` 포트 번호 확인 (기본: 12345)

### 문제: 아바타의 blendshape가 적용되지 않음
- **해결**: `AvatarBlendshapeController`의 `BuildBlendshapeMap()`에서 매핑 확인
- **해결**: GLB 모델의 BlendShape 이름이 MediaPipe 이름과 일치하는지 확인

### 문제: Unity 렌더링 이미지가 Qt에 표시되지 않음
- **해결**: `RenderTextureSender`의 연결 포트 확인 (기본: 12346)
- **해결**: Qt의 `unityImageServer`가 올바른 포트에서 리스닝하는지 확인

## 9. 참고 자료

- [MediaPipeUnityPlugin GitHub](https://github.com/homuler/MediaPipeUnityPlugin)
- [Unity BlendShape 문서](https://docs.unity3d.com/Manual/class-SkinnedMeshRenderer.html)
- [MediaPipe Face Landmarker 문서](https://ai.google.dev/edge/mediapipe/solutions/vision/face_landmarker)
