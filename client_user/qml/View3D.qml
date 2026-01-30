import QtQuick
import QtQuick3D
import QtQml 

View3D {
    id: view3D
    anchors.fill: parent

    // C++에서 받는 데이터 연결
    property int avatarIndex: videoWidget ? videoWidget.avatarIndex : -1
    property string glbModelPath: videoWidget ? videoWidget.glbModelPath : ""
    property var blendshapes: videoWidget ? videoWidget.blendshapes : []
    
    // 얼굴 위치 정보
    property real faceX: videoWidget ? videoWidget.faceX : 0.0
    property real faceY: videoWidget ? videoWidget.faceY : 0.0
    property real faceWidth: videoWidget ? videoWidget.faceWidth : 0.0
    property real faceHeight: videoWidget ? videoWidget.faceHeight : 0.0

    environment: SceneEnvironment {
        clearColor: "#111111"
        backgroundMode: SceneEnvironment.Color
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    PerspectiveCamera {
        id: camera
        position: Qt.vector3d(0, 0, 80)
        fieldOfView: 30
    }

    DirectionalLight {
        eulerRotation: Qt.vector3d(-30, -30, 0)
        brightness: 1.5
    }
    PointLight {
        position: Qt.vector3d(0, 0, 100)
        brightness: 0.5
    }

    Node {
        id: avatarNode

        Model {
            id: avatarModel
            objectName: "avatarModel"
            source: glbModelPath
            visible: glbModelPath !== ""
            
            scale: Qt.vector3d(15.0, 15.0, 15.0)
            position: Qt.vector3d(0, -10, 0)

            materials: [
                PrincipledMaterial {
                    baseColor: "white"
                    roughness: 0.5
                    metalness: 0.0 // metallic 아님 주의
                }
            ]

            // [핵심 해결] 
            // 1. morphTargetWeights 속성은 없으니 쓰지 않습니다.
            // 2. 52개를 다 만들면 터지니까 8개까지만 만듭니다.
            morphTargets: ListModel {
                 id: morphList
            }

            Instantiator {
                // ▼▼▼ 중요: 52가 아니라 8로 제한합니다! ▼▼▼
                model: 8 
                delegate: MorphTarget {
                    // MediaPipe 데이터(blendshapes)와 GLB MorphTarget을 1:1로 매핑
                    // GLB 파일의 0~7번 MorphTarget만 제어합니다.
                    weight: (view3D.blendshapes && view3D.blendshapes.length > index) 
                            ? view3D.blendshapes[index] 
                            : 0.0
                }
                onObjectAdded: (index, object) => {
                    avatarModel.morphTargets.push(object)
                }
            }
        }
    }
}