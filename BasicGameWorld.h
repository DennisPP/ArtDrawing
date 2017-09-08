#ifndef __BasicGameWorld__h__
#define __BasicGameWorld__h__

#include "GameWorldBase.h"

#include <vector>
#include <map>
#include <string>

#include "Math3D/Vector2.h"
#include "Math3D/Vector4.h"
#include "Math3D/Matrix4.h"
#include "Math3D/Plane.h"
#include "Math3D/Ray.h"

#include "Graphics/Color.h"
#include "Spline.h"

#include "Graphics/TextureInfo.h"

#include "AnimalsState.h"

class TOrbitCamera;
class SkyBoxRenderer;
class GameObject;
class MainController;
class AnimalsState;
class SteerObject;
class ZooManager;
class Monster;

class TTouchControl;


class BubbleEmitter;


class LightRay
{
public:
    LightRay();
    float angle;
};


class BasicGameWorld : public GameWorldBase
{
public:
    
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
    
    WorkingMode workingMode;
    
    
    BasicGameWorld(AnimalsState* animalsState);
    virtual bool Create();
    virtual void Destroy();
    virtual void Update(double dt);
    virtual void Render(const Matrix4& viewMatrix, const Matrix4& projectionMatrix);
    
    virtual void OnTouchDown(const std::vector<TTouchControl>& touches);
    virtual void OnTouchMoved(const std::vector<TTouchControl>& touches);
    virtual void OnTouchUp(const std::vector<TTouchControl>& touches);
    virtual void OnScrollWheel(float delta);
    virtual void OnPinched(float scale, float x, float y, float dx, float dy);
    
    
    virtual void OnResize(float width, float height);
    
    void FixCameraPositionFromScreenResolution();

    
    void RenderSteerObject(SteerObject* s);
    void RenderMessages();
    void RenderSpline(TSpline* s);
    void RenderMonster(Monster* monster);
    void RenderMonsterMessage(Monster* monster);
    void Restart();
    
    void LoadPaths();
    void LoadSettings();
    
    
    
    
    
    int GetInZooAnimalCount() const;
    int GetLeavingZooAnimalCount() const;
    
  
    void ResolveCollsion();
    
    float GetTotalPlayedTime() const;
    
  
    std::vector<GameObject*> gameObjects;
    std::vector<GameObject*> spiders;
    
    
    
    AnimalsState* animalsState;
    
    ZooManager* zooManager;
    float totalPlayedTime;
    
    bool IsInside(const Vector2& pos) const;
    
    bool EvalSpace(GameObject* g) const;
    
    
    void UpdateCameraFromVR();

    
    void CreateMonsterIfNOTExist(const std::string& user, const std::string& displayName, const std::string& createdate, int animalType);
    void UpdateMonstersTexture(const std::string& user, int animalType, unsigned int texture, const std::string& createDate);  
    

    
    GameObject* GetMonsterByUserId(const std::string& user, int animalType);
    ViewMode viewMode;
    
    
    std::vector<TSpline*> splines;
    std::vector<TSpline*> starSplines;
    
    std::vector<Monster*> starMonsters;
    Monster* GetFreeStarMonster() const;
    
    
    Vector2 minPt;
    Vector2 maxPt;
    
    
    unsigned int backgroundTexture;
    
    float skyBoxSize;
    
    bool playerShouldMove;
    Vector3 playerPos;
    
    
    bool drawPath;
    Matrix4 CalculateMonsterTransform(Monster* monster);
    
    Matrix4 inverseViewMatrix;
    
    Plane frustumPlanes[6];
    int renderedMonsterCount;
    int frustumCulledCount;
    std::map<std::string, TTextureInfo> texturesMap;
    
    bool willUseSkyBox;
    SkyBoxRenderer* skyBoxRenderer;

    TOrbitCamera* mOrbitCamera;
    
    Settings settings;
    
    Matrix4 viewMatrix;
    ViewParam initialViewParam;
    
    Ray CalPickRay(int mx, int my);
    
    
    void CreateBubbles();
    std::vector<BubbleEmitter*> bubbleEmitters;
    
    bool willRenderCaustics;
    
    std::vector<LightRay> lightRays;
    unsigned int lightTexture;
    unsigned int causticTextures[32];
};


#endif
