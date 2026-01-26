import QtQuick 2.15
import QtQuick3D 1.15

View3D {
    id: view3D
    anchors.fill: parent
    camera: camera
    
    property int avatarIndex: -1
    property string glbModelPath: ""
    property real faceX: 0.0
    property real faceY: 0.0
    property real faceWidth: 0.0
    property real faceHeight: 0.0
    
    // 카메라 설정
    PerspectiveCamera {
        id: camera
        position: Qt.vector3d(0, 0, 5)
        eulerRotation: Qt.vector3d(0, 0, 0)
    }
    
    // 조명
    DirectionalLight {
        id: light
        eulerRotation: Qt.vector3d(-45, 45, 0)
        brightness: 1.0
    }
    
    // 3D 아바타 모델
    Node {
        id: avatarNode
        visible: avatarIndex >= 0
        
        // GLB 모델 로드
        Model {
            id: avatarModel
            source: glbModelPath !== "" ? glbModelPath : ""
            visible: glbModelPath !== ""
            
            // 얼굴 위치에 배치
            position: Qt.vector3d(
                (faceX - 0.5) * 2.0,  // -1 ~ 1 범위로 변환
                -(faceY - 0.5) * 2.0, // Y축 뒤집기
                0.0
            )
            
            // 얼굴 크기에 맞춰 스케일
            scale: Qt.vector3d(
                faceWidth * 2.0,
                faceHeight * 2.0,
                1.0
            )
            
            // 머티리얼
            materials: [
                PrincipledMaterial {
                    baseColor: getAvatarColor()
                    metallic: 0.0
                    roughness: 0.5
                }
            ]
        }
        
        // GLB 파일이 없을 때 기본 큐브
        Model {
            id: defaultCube
            source: "#Cube"
            visible: glbModelPath === "" && avatarIndex >= 0
            
            position: Qt.vector3d(
                (faceX - 0.5) * 2.0,
                -(faceY - 0.5) * 2.0,
                0.0
            )
            
            scale: Qt.vector3d(
                faceWidth * 2.0,
                faceHeight * 2.0,
                1.0
            )
            
            materials: [
                PrincipledMaterial {
                    baseColor: getAvatarColor()
                    metallic: 0.0
                    roughness: 0.5
                }
            ]
        }
    }
    
    // 캐릭터별 색상 반환
    function getAvatarColor() {
        var colors = [
            Qt.rgba(1.0, 0.0, 0.0, 1.0),  // 빨강
            Qt.rgba(0.0, 1.0, 0.0, 1.0),  // 초록
            Qt.rgba(0.0, 0.0, 1.0, 1.0),  // 파랑
            Qt.rgba(1.0, 1.0, 0.0, 1.0)   // 노랑
        ];
        
        if (avatarIndex >= 0 && avatarIndex < colors.length) {
            return colors[avatarIndex];
        }
        return Qt.rgba(1.0, 1.0, 1.0, 1.0);
    }
}
