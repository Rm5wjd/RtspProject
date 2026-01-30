import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    id: root
    anchors.fill: parent
    
    // C++에서 전달받을 property들 (videoWidget context property로 전달)
    property int avatarIndex: videoWidget ? videoWidget.avatarIndex : -1
    property string glbModelPath: videoWidget ? videoWidget.glbModelPath : ""
    property real faceX: videoWidget ? videoWidget.faceX : 0.0
    property real faceY: videoWidget ? videoWidget.faceY : 0.0
    property real faceWidth: videoWidget ? videoWidget.faceWidth : 0.0
    property real faceHeight: videoWidget ? videoWidget.faceHeight : 0.0
    
    // 비디오 배경 (2D) - C++ ImageProvider에서 가져온 프레임 표시
    Image {
        id: videoBackground
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "image://video/frame"
        cache: false          // 실시간 업데이트 위해 캐시 비활성화
        asynchronous: false
        smooth: false         // 성능 우선

        // 주기적으로 이미지 갱신 (~30 FPS)
        Timer {
            id: videoUpdateTimer
            interval: 33      // 33ms ≒ 30 FPS
            running: true
            repeat: true
            onTriggered: {
                // 쿼리 스트링에 타임스탬프를 붙여 강제로 reload
                var ts = Date.now()
                videoBackground.source = "image://video/frame?" + ts
            }
        }

        // 초기 로드
        Component.onCompleted: {
            videoBackground.source = "image://video/frame?" + Date.now()
        }
    }
    
    // 3D 씬 로더 (Quick3D가 있을 때만) - Image 위에 렌더링되도록 z-order 조정
    Loader {
        id: scene3DLoader
        anchors.fill: parent
        z: 1  // Image 위에 렌더링 (Image는 기본 z:0)
        source: quick3DAvailable ? "View3D.qml" : ""
        
        property bool quick3DAvailable: false
        
        Component.onCompleted: {
            // Quick3D 사용 가능 여부 확인 (런타임)
            try {
                var testComponent = Qt.createComponent("View3D.qml")
                if (testComponent.status === Component.Ready) {
                    quick3DAvailable = true
                    source = "View3D.qml"
                    console.log("[VideoScene] Quick3D available, loading View3D.qml")
                } else {
                    console.log("[VideoScene] Quick3D component status:", testComponent.status, testComponent.errorString())
                }
            } catch(e) {
                console.log("[VideoScene] Qt Quick3D not available:", e)
                quick3DAvailable = false
            }
        }
        
        onLoaded: {
            if (item) {
                console.log("[VideoScene] View3D loaded via Loader. avatarIndex=", root.avatarIndex, "glbModelPath=", root.glbModelPath)
                item.avatarIndex = Qt.binding(function() { return root.avatarIndex })
                item.glbModelPath = Qt.binding(function() { return root.glbModelPath })
                item.faceX = Qt.binding(function() { return root.faceX })
                item.faceY = Qt.binding(function() { return root.faceY })
                item.faceWidth = Qt.binding(function() { return root.faceWidth })
                item.faceHeight = Qt.binding(function() { return root.faceHeight })
                // blendshapes도 명시적으로 바인딩 (View3D.qml 내부에서 videoWidget 참조가 꼬일 때 대비)
                if (item.hasOwnProperty("blendshapes")) {
                    item.blendshapes = Qt.binding(function() { return videoWidget ? videoWidget.blendshapes : [] })
                }
            } else {
                console.log("[VideoScene] View3D Loader item is null")
            }
        }
    }
    
    // 얼굴이 탐지되었을 때 2D 표시 (Quick3D 여부와 무관하게)
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        visible: faceWidth > 0 && faceHeight > 0
        
        Rectangle {
            x: (parent.width * faceX) - (parent.width * faceWidth / 2)
            y: (parent.height * faceY) - (parent.height * faceHeight / 2)
            width: parent.width * faceWidth
            height: parent.height * faceHeight
            color: getAvatarColor()
            border.color: "white"
            border.width: 2
            radius: 10
            
            // 간단한 얼굴 표시
            Row {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -parent.height * 0.15
                spacing: parent.width * 0.2
                
                // 눈
                Rectangle {
                    width: parent.parent.width * 0.15
                    height: width
                    radius: width / 2
                    color: "white"
                }
                Rectangle {
                    width: parent.parent.width * 0.15
                    height: width
                    radius: width / 2
                    color: "white"
                }
            }
            
            // 입
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.verticalCenter
                anchors.topMargin: parent.height * 0.2
                width: parent.width * 0.3
                height: width * 0.5
                radius: width / 2
                color: "white"
            }
        }
    }
    
    // 캐릭터별 색상 반환
    function getAvatarColor() {
        var colors = [
            Qt.rgba(1.0, 0.0, 0.0, 0.7),  // 빨강
            Qt.rgba(0.0, 1.0, 0.0, 0.7),  // 초록
            Qt.rgba(0.0, 0.0, 1.0, 0.7),  // 파랑
            Qt.rgba(1.0, 1.0, 0.0, 0.7)   // 노랑
        ];
        
        if (avatarIndex >= 0 && avatarIndex < colors.length) {
            return colors[avatarIndex];
        }
        return Qt.rgba(1.0, 1.0, 1.0, 0.7);
    }
}
