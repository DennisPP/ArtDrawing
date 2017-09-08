#ifndef __AnimalsState__h__
#define __AnimalsState__h__
#include <stdio.h>

#include "fbxsdk.h"


#include "OBJModel.h"


#include "Math3D/Ray.h"
#include "Graphics/Color.h"

class TGameApp;
class TTouchControl;

class TOrbitCamera;

class FBXLoader;

class TSpline;

#include <vector>
#include <string>
#include "GameState.h"

#include "SkyBoxRenderer.h"

class Monster;
class MainController;

class GameWorldBase;
class SkyBox;

class TSprite;
class TerrainRenderer;

class ViewParam
{
public:
    ViewParam();
    ViewParam(const Vector3& eye, const Vector3& at, const Vector3& up);
    
    
    static ViewParam Make16To9ViewParam();
    

    Vector3 eye;
    Vector3 at;
    Vector3 up;
};


class StudentInfo
{
public:
    StudentInfo();
    StudentInfo(const std::string& className, const std::string& name);    
    std::string className;
    std::string name;
    std::string texturePath;
    std::string displayName;
    unsigned int texture;
    int animalType;
};

class AnimalsState : public GameState
{
    class Settings
    {
    public:
        Settings();
        
        
        float starFrequency;
        
        
        int fontSize;
        
        float fovy;
        float nearPlane;
        float farPlane;
        
        TColor nameTagBackgroundColor;
        TColor nameTagTextColor;
        float nameTagOffset;
        bool isFixedCamera;
        
        
        Vector3 defaultCameraEye;
        Vector3 defaultCameraAt;
        Vector3 defaultCameraUp;
        
        
        bool useOfflineData;
        
        float skyBoxSize;
    };
    
    typedef enum
    {
        WorkingModePlayMode = 0,
        WorkingModeEditMode
    } WorkingMode;
    
    
    typedef enum
    {
        ViewMode2DDisplay = 0,
        ViewModeVR,
        ViewMode3DDebug,
    
    } ViewMode;
    
    
    public:
    AnimalsState(MainController* controller);
    bool Create();
    void Update(double dt);
    void Render();
    void Reset();
    void OnResize(float width, float height);
    
    
    
    virtual void Destroy();
    
    virtual void OnTouchDown(const std::vector<TTouchControl>& touches);
    virtual void OnTouchMoved(const std::vector<TTouchControl>& touches);
    virtual void OnTouchUp(const std::vector<TTouchControl>& touches);
    virtual void OnScrollWheel(float delta);
    
    
    
    void OnPinched(float scale, float x, float y, float dx, float dy);
    
    void RenderFloor();
    bool willRenderFloor;
    
    virtual void OnEnter();
    virtual void OnLeave();
    
    
    void CreateGameObjects();
    Vector2 ToScreenPosition(const Vector3& worldPt) const;
    
    void LoadStudentInfo();
    

    
    float GetTotalPlayedTime() const;
    void ResetToDefaultCameraSettings();
    
    
    
    
    
    
    unsigned int mVBO, mIBO;
    int triangleCount;
    
    
    ViewParam initialViewParam;
    void FixCameraPositionFromScreenResolution();
    
    
    std::string LoadFile(const std::string &path);
    TSpline* LoadSpline(const std::string &path);
    
    Settings settings;
    
    Ray CalPickRay(int mx, int my);

    
    void DumpNode(FbxNode* node, int depth=0);
    
    
    FbxTime animationDuration;
    unsigned int whiteTexture;
    unsigned int floorTexture;
    
    //FBXLoader* fbxModels[3];
    unsigned int animalTextures[4];
    
    
    //SkyBox* skyBox;
    
    
    GameWorldBase* gameWorld;
    
    float totalPlayedTime;
    
    std::vector<StudentInfo> studentInfos;
    std::vector<Vector3> controlPoints;
    WorkingMode workingMode;
    void PrintControlPoints();
    
    
    TSprite* activityLogoSprite;
    
    
    bool willUseSkyBox;
    SkyBoxRenderer* skyBoxRenderer;

    
    virtual void OnVREventFired();

    
    void UpdateCameraFromVR();
    void UpdatePlayer(double dt);
    Vector3 playerPos;
    bool playerShouldMove;
    
    
    Matrix4 viewMatrix;
    ViewMode viewMode;
    
    bool willDrawTerrain;
    TerrainRenderer* terrainRenderer;
    unsigned int terrainTexture;
    
    
    
};

#endif
