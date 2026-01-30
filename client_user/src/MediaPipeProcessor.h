#ifndef MEDIAPIPEPROCESSOR_H
#define MEDIAPIPEPROCESSOR_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <opencv2/opencv.hpp>

/**
 * MediaPipe Python 임베딩 프로세서 (pybind11 embed)
 *
 * - 같은 프로세스 내에서 Python을 임베딩하고(`pybind11::embed`)
 * - `client_user/python/mediapipe_module.py`의 함수들을 직접 호출합니다.
 */
class MediaPipeProcessor : public QObject
{
    Q_OBJECT

public:
    struct FaceData {
        struct Landmark {
            float x, y, z;
        };
        
        struct Blendshape {
            QString category;
            float score;
        };
        
        QVector<Landmark> landmarks;  // 468개 랜드마크
        QVector<Blendshape> blendshapes;  // 52개 Blendshape
    };
    
    explicit MediaPipeProcessor(QObject *parent = nullptr);
    ~MediaPipeProcessor();
    
    /**
     * MediaPipe 프로세서 시작
     * @return 성공 여부
     */
    bool start();
    
    /**
     * MediaPipe 프로세서 종료
     */
    void stop();
    
    /**
     * 프레임을 큐에 추가하여 처리 요청
     * 
     * 주의: 이 클래스는 카메라를 직접 열지 않습니다.
     * MainWindow에서 카메라를 열고, 읽은 프레임을 이 함수로 전달해야 합니다.
     * 
     * @param frame OpenCV Mat 프레임 (MainWindow에서 videoCapture.read()로 읽은 프레임)
     */
    void processFrame(const cv::Mat &frame);
    
    /**
     * 처리 간격 설정 (N프레임마다 처리)
     */
    void setProcessInterval(int interval);
    
    /**
     * MediaPipe 프로세서가 실행 중인지 확인
     */
    bool isRunning() const;

signals:
    /**
     * 얼굴 탐지 결과가 준비되었을 때 발생
     * @param faces 얼굴 데이터 배열
     */
    void faceDetected(const QVector<FaceData> &faces);
    
    /**
     * 오류 발생 시 발생
     * @param error 오류 메시지
     */
    void errorOccurred(const QString &error);

private slots:
    void processNextFrame();

private:
    /**
     * mediapipe_module.py 경로(디렉토리)를 찾아 sys.path에 추가하기 위함
     */
    QString findPythonModuleDir() const;

    /**
     * MediaPipe 모델 파일(face_landmarker.task) 경로 찾기
     */
    QString findModelPath() const;

    /**
     * Python 환경 초기화 + 모듈/함수 로드
     */
    bool ensurePythonReady();
    
    /**
     * 큐에서 다음 프레임 처리
     */
    void processQueuedFrame();
    
    QQueue<cv::Mat> m_frameQueue;
    QMutex m_queueMutex;
    QTimer *m_processTimer;
    
    int m_processInterval;  // N프레임마다 처리
    int m_frameCounter;
    bool m_isProcessing;
    bool m_running;
    bool m_pythonReady;
    bool m_needsReset;
    
    static const int MAX_QUEUE_SIZE = 5;  // 최대 큐 크기 (오래된 프레임 버림)
};

#endif // MEDIAPIPEPROCESSOR_H
