#include "BasicGameWorld.h"

#include "MainController.h"
#include "AnimalsState.h"

#include "GameObject.h"
#include "Monster.h"
#include "ZooManager.h"
#include "StaticObject.h"

#include "FBXLoader.h"


#include "SystemUtils.h"

#include "OrbitCamera.h"

#include "Core/GremlinsFramework.h"
#include "Core/Device.h"


#include "Graphics/Line3DRenderer.h"
#include "Graphics/FlatRenderer.h"
#include "Graphics/Renderer.h"

#include "Graphics/TextureManager.h"

#include "Math3D/AABB.h"
#include "Math3D/MathUtil.h"
#include "TerrainRenderer.h"

#include "GameStatics.h"
#include "BubbleEmitter.h"



#include "tinyxml.h"


LightRay::LightRay()
{}


BasicGameWorld::Settings::Settings()
{
    starFrequency = 4.5f;
    
    //TODO:
    useOfflineData = false;
    
    skyBoxSize = 4000.0f;
    
    
    nameTagOffset = -60.0f;
    
    isFixedCamera = false;
    
    
    fovy = 60.0f;
    nearPlane = 1.0f;
    farPlane = 40000.0f;
    
    fontSize = 14;
    
    nameTagBackgroundColor = TColor::FromIntColor(250, 250, 250, 255);
    nameTagBackgroundColor.a = 0.4f;
    nameTagTextColor = TColor::FromIntColor(100, 100, 100, 255);
    
    
    defaultCameraEye = Vector3(0.000000f, 4950.302246f, 4513.788086f);
    defaultCameraAt = Vector3(0.000000f, -400.000000f, 0.000000f);
    defaultCameraUp = Vector3(0.000000f, 0.644827f, -0.764329f);
}



BasicGameWorld::BasicGameWorld(AnimalsState* state)
:   GameWorldBase()
,   animalsState(state)
{
    totalPlayedTime = 0.0f;
    drawPath = false;
}

bool BasicGameWorld::Create()
{
    
    
    
    workingMode = WorkingModePlayMode;
    
    float r = 1450.0f;
    if( viewMode == ViewMode::ViewMode3DDebug )
      r = 20000.0f;
    
    

    
    //mOrbitCamera = new TOrbitCamera(Vector3(0.0f, 400.0f, 0.0f), r);
    mOrbitCamera = new TOrbitCamera(Vector3(0.0f, -400.0f, 0.0f), r);
    mOrbitCamera->canRotateAroundRight = true;
    mOrbitCamera->canRotateAroundUp = false;
    

    
    viewMode = ViewMode::ViewMode2DDisplay;
    //viewMode = ViewMode::ViewMode3DDebug;
    
    
    if( viewMode==ViewMode::ViewMode3DDebug )
    {
        r = 16000.0f;
        mOrbitCamera->LookAt(Vector3(0.0f, 0.0f, r), Vector3::ZERO, Vector3::UNIT_Y);
        float s = 18000.0f;
        settings.skyBoxSize = s;
        settings.farPlane = 1.0f + sqrtf(4*s*s + 4*s*s);
    }

    
    skyBoxSize = 4000.0f;
    
    playerPos = Vector3(0.0f, 100.0f, 0.0f);
    playerShouldMove = false;
    
    
    totalPlayedTime = 0.0f;
    
    const float halfRange = 2000.0f;
    minPt = Vector2(-halfRange, -halfRange);
    maxPt = Vector2(halfRange, halfRange);
    
    
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    int buildType = SystemUtils::GetBuildType();
    if( buildType==2)
    {
        backgroundTexture = TTextureManager::GetInstance().GetTexture((root + "media/textures/background16_9.png").c_str()).TextureID;
    }
    else
    {
        backgroundTexture = TTextureManager::GetInstance().GetTexture((root + "media/textures/background.png").c_str()).TextureID;
    }
    
    
    willRenderCaustics = buildType == 2;
    #if 1
    if( willRenderCaustics )
    {
        for(int j=0;j<32;++j)
        {
            char path[1024];
            sprintf(path, "%smedia/textures/caustic/caust%.2d.png", root.c_str(), j);
            causticTextures[j] = TTextureManager::GetInstance().GetTexture(path).TextureID;
        }
        lightTexture = causticTextures[0];
    }
    #endif
    
    
    skyBoxRenderer = NULL;
    willUseSkyBox = viewMode == ViewMode::ViewMode3DDebug;
#if VR_BUILD
    int buildType = SystemUtils::GetBuildType();
    willUseSkyBox = buildType==0 || buildType==3;
#endif
    
    if( willUseSkyBox )
    {
        std::vector<std::string> paths(6);
        assert( paths.size()==6);
        
        paths[0] = root+"media/textures/skybox/skybox_pos_x.png";
        paths[1] = root+"media/textures/skybox/skybox_neg_x.png";
        paths[2] = root+"media/textures/skybox/skybox_pos_y.png";
        paths[3] = root+"media/textures/skybox/skybox_neg_y.png";
        paths[4] = root+"media/textures/skybox/skybox_pos_z.png";
        paths[5] = root+"media/textures/skybox/skybox_neg_z.png";
        
        
#if 1
        paths[0] = root+"media/textures/xpos.png";
        paths[1] = root+"media/textures/xneg.png";
        paths[2] = root+"media/textures/ypos.png";
        paths[3] = root+"media/textures/yneg.png";
        paths[4] = root+"media/textures/zpos.png";
        paths[5] = root+"media/textures/zneg.png";
        
#endif
        
        skyBoxRenderer = new SkyBoxRenderer();
        skyBoxRenderer->Create(paths);
    }

    

    const int maxStarMonster = 20;
    for(int j=0;j<maxStarMonster;++j)
    {
        Monster * monster = new Monster(this);
        starMonsters.push_back(monster);
    }
    
    //LoadPaths();
    
    
#if 1
    
    int studentCount = (int)animalsState->studentInfos.size();
    
    int index = 0;
    for(int j=0;j<studentCount;++j)
    {
        Monster* monster = new Monster(this);
        
        
        monster->SetAnimalType(animalsState->studentInfos[j].animalType % 4);
        monster->displayName = animalsState->studentInfos[j].displayName;
        monster->texture = animalsState->studentInfos[j].texture;
        
        // lion move faster
        monster->position = Vector2(Math::RangeRandom(minPt.x, maxPt.x), Math::RangeRandom(minPt.y, maxPt.y));
        //monster->StartWandering();
        monster->SetName(std::to_string(index) +" : " + animalsState->studentInfos[j].className + " : " + animalsState->studentInfos[j].name);
        
        if( SystemUtils::buildType==1 && monster->GetAnimalType()==2 )
        {
            // Spider
            spiders.push_back(monster);
        }
        else
        {
            gameObjects.push_back(monster);
        }
        
        ++index;
    }
    
    printf("%d object loaded! \n", (int)gameObjects.size());
    printf("%d spiders loaded! \n", (int)spiders.size());
    
    
    for(int j=0;j<gameObjects.size();++j)
    {
        gameObjects[j]->SetMessage(gameObjects[j]->GetName());
    }
    
#endif
    
    {
        std::string pathPath = root + "media/paths/star_path01.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        starSplines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/star_path02.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        starSplines.push_back(spline);
    }
    
    
    

    
    LoadSettings();
    
    
    
    
    initialViewParam.eye = settings.defaultCameraEye;
    initialViewParam.at  = settings.defaultCameraAt;
    initialViewParam.up  = settings.defaultCameraUp;
    
    
    zooManager = new ZooManager(this);
    zooManager->Start();
    
    
    

    
    
    if( 2==SystemUtils::GetBuildType())
    {
        CreateBubbles();
    }
    
    return true;
}

void BasicGameWorld::Destroy()
{
    SAFE_DELETE(zooManager);       
}


void BasicGameWorld::CreateBubbles()
{
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    
    unsigned int bubbleTexture = TTextureManager::GetInstance().GetTexture( (root+"media/textures/bubble_512.png").c_str()).TextureID;
   
    #if 0
    {
        BubbleParam param;
        param.gravity = -900.0f;
        param.minSpeed = 1000.0f;
        param.maxSpeed = 2000.0f;
        BubbleEmitter* emitter = new BubbleEmitter(bubbleTexture, param);
        emitter->SetEmitPerSecond(30.0f);
        float x = -2400.0f;
        float z = 2000.0f;
        float y = 0.0f;//terrainRenderer->GetHeight(x,z);
        Matrix4 t = Matrix4::getTrans(Vector3(x, y, z));
        emitter->SetTransform(t);
        bubbleEmitters.push_back(emitter);
    }
    #endif
    

    {
        BubbleEmitter* emitter = new BubbleEmitter(bubbleTexture);
        emitter->SetEmitPerSecond(4.0f);
        float x = -1800.0f;
        float z = 1800.0f;
        float y = 0.0f;//terrainRenderer->GetHeight(x,z);
        Matrix4 t = Matrix4::getTrans(Vector3(x, y, z));
        emitter->SetTransform(t);
        bubbleEmitters.push_back(emitter);
    }
    
    {
        BubbleEmitter* emitter = new BubbleEmitter(bubbleTexture);
        emitter->SetEmitPerSecond(6.0f);
        float x = 0.0f;
        float z = 2000.0f;
        float y = 0.0f;//terrainRenderer->GetHeight(x,z);
        Matrix4 t = Matrix4::getTrans(Vector3(x, y, z));
        emitter->SetTransform(t);
        bubbleEmitters.push_back(emitter);
    }
    {
        BubbleEmitter* emitter = new BubbleEmitter(bubbleTexture);
        emitter->SetEmitPerSecond(6.0f);
        float x = 1600.0f;
        float z = 2000.0f;
        float y = 0.0f;//terrainRenderer->GetHeight(x,z);
        Matrix4 t = Matrix4::getTrans(Vector3(x, y, z));
        emitter->SetTransform(t);
        bubbleEmitters.push_back(emitter);
    }
    
}

void BasicGameWorld::UpdateMonstersTexture(const std::string& user, int animalType, unsigned int texture, const std::string& createDate)
{
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(gameObjects[j]);
        if( monster && 0==monster->userId.compare(user) && monster->GetAnimalType()==animalType)
        {
            monster->texture = texture;
            monster->createdate = createDate;
        }
    }
    for(int j=0;j<(int)starMonsters.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(starMonsters[j]);
        if( monster && 0==monster->userId.compare(user) && monster->GetAnimalType()==animalType)
        {
            monster->texture = texture;
        }
    }
    for(int j=0;j<(int)spiders.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(spiders[j]);
        if( monster && 0==monster->userId.compare(user) && monster->GetAnimalType()==animalType)
        {
            monster->texture = texture;
        }
    }
}

void BasicGameWorld::CreateMonsterIfNOTExist(const std::string &user, const std::string& displayName, const std::string& createdate, int animalType)
{
    if( GetMonsterByUserId(user, animalType))
        return;
    
    assert(animalType>=0 && animalType<4);
    Monster* monster = new Monster(this);
    monster->userId = user;
    monster->createdate = createdate;
    monster->SetAnimalType(animalType);
    monster->displayName = displayName;
    monster->texture = animalsState->animalTextures[animalType];
    
    // lion move faster
    monster->position = Vector2(Math::RangeRandom(minPt.x, maxPt.x), Math::RangeRandom(minPt.y, maxPt.y));
    monster->SetName(monster->displayName);
    if( SystemUtils::buildType==1 && monster->GetAnimalType()==2)
    {
        spiders.push_back(monster);
    }
    else
    {
        gameObjects.push_back(monster);
    }
}


GameObject* BasicGameWorld::GetMonsterByUserId(const std::string& user, int animalType)
{
    //TODO:
    for(int j=0;j<gameObjects.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(gameObjects[j]);        
        if( monster && 0==monster->userId.compare(user) && animalType == monster->GetAnimalType())
            return monster;        
    }
    for(int j=0;j<spiders.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(spiders[j]);
        if( monster && 0==monster->userId.compare(user) && animalType ==monster->GetAnimalType())
            return monster;
    }
    return NULL;
}


void BasicGameWorld::LoadSettings()
{
    printf("%s\n", __FUNCTION__);
    
    
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    const std::string path = (root + "media/settings.xml");
    
    TiXmlDocument doc;
    doc.LoadFile(path.c_str());
    
    TiXmlElement * rootElement = doc.FirstChildElement("Root");
    assert( rootElement );
    
    double starFrequency = 4.5f;
    if( rootElement->Attribute("starFrequency", &starFrequency))
    {
        settings.starFrequency = starFrequency;
    }    
    
    TiXmlElement* nameTagElement = rootElement->FirstChildElement("NameTag");
    if( nameTagElement )
    {
        TiXmlElement* backgroundColorElement = nameTagElement->FirstChildElement("BackgroundColor");
        if( backgroundColorElement )
        {
            int r = 255, g = 255, b = 255, a = 255;
            backgroundColorElement->Attribute("r", &r);
            backgroundColorElement->Attribute("g", &g);
            backgroundColorElement->Attribute("b", &b);
            backgroundColorElement->Attribute("a", &a);
            animalsState->settings.nameTagBackgroundColor = TColor::FromIntColor(r, g, b, a);
        }
        TiXmlElement* textColorElement = nameTagElement->FirstChildElement("TextColor");
        if( textColorElement )
        {
            int r = 255, g = 255, b = 255, a = 255;
            textColorElement->Attribute("r", &r);
            textColorElement->Attribute("g", &g);
            textColorElement->Attribute("b", &b);
            textColorElement->Attribute("a", &a);
            animalsState->settings.nameTagTextColor = TColor::FromIntColor(r, g, b, a);
        }
        TiXmlElement* offsetElement = nameTagElement->FirstChildElement("Offset");
        if( offsetElement )
        {
            double offset = 0.0f;
            offsetElement->Attribute("y", &offset);
            animalsState->settings.nameTagOffset = offset;
        }
    }
    
    
    TiXmlElement* cameraElement = rootElement->FirstChildElement("Camera");
    if( cameraElement )
    {
        TiXmlElement* eyeElement = cameraElement->FirstChildElement("Eye");
        TiXmlElement*  atElement = cameraElement->FirstChildElement("At");
        TiXmlElement*  upElement = cameraElement->FirstChildElement("Up");
        
        if( eyeElement && atElement && upElement )
        {
            double eye[3], at[3], up[3];
            eyeElement->Attribute("x", &eye[0]);
            eyeElement->Attribute("y", &eye[1]);
            eyeElement->Attribute("z", &eye[2]);
            
            atElement->Attribute("x", &at[0]);
            atElement->Attribute("y", &at[1]);
            atElement->Attribute("z", &at[2]);
            
            upElement->Attribute("x", &up[0]);
            upElement->Attribute("y", &up[1]);
            upElement->Attribute("z", &up[2]);
            Vector3 e(eye[0], eye[1], eye[2]);
            Vector3 a(at[0], at[1], at[2]);
            Vector3 u(up[0], up[1], up[2]);
            mOrbitCamera->LookAt(e, a, u);
            settings.defaultCameraEye = e;
            settings.defaultCameraAt = a;
            settings.defaultCameraUp = u;
        }
    }
    
    
    TiXmlElement* splineElement = rootElement->FirstChildElement("Spline");
    for(;splineElement;splineElement=splineElement->NextSiblingElement("Spline"))
    {
        const char* path = splineElement->Attribute("path");
        int isStar = 0;
        int star = 0;
        if( splineElement->Attribute("star", &star))
        {
            isStar = star;
        }
        if( path )
        {
            const std::string fullPath = root + "media/" + path;
            TSpline* spline = animalsState->LoadSpline(fullPath);
            if( isStar )
                starSplines.push_back(spline);
            else
                splines.push_back(spline);
        }
    }
    
    TiXmlElement* resourceElement = rootElement->FirstChildElement("Resource");
    if( resourceElement )
    {
        TiXmlElement* textureElement = resourceElement->FirstChildElement("Texture");
        int loadedTexture = 0;
        for(;textureElement;textureElement=textureElement->NextSiblingElement("Texture"))
        {
            const char* name = textureElement->Attribute("name");
            const char* path = textureElement->Attribute("path");
            if( name && path )
            {
                const std::string fullPath = root + "media/" + path;
                TTextureInfo info = TTextureManager::GetInstance().GetTexture(fullPath.c_str());
                texturesMap[std::string(name)] = info;
                ++ loadedTexture;
            }
        }
        printf("%d texture loaded\n", loadedTexture);
    }    
    
    TiXmlElement* objectsElement = rootElement->FirstChildElement("Objects");
    if( objectsElement )
    {
        TiXmlElement* treeElement = objectsElement->FirstChildElement("Tree");
        std::vector<std::string> failedTextures;
        for(;treeElement;treeElement=treeElement->NextSiblingElement("Tree"))
        {
            const char* texture = treeElement->Attribute("texture");
            if( ! texture )
            {
            }
            else
            {
                std::map<std::string, TTextureInfo>::iterator it = texturesMap.find(std::string(texture));
                if( it != texturesMap.end())
                {
                    const TTextureInfo& info = it->second;
                    double x =0.0f, y=0.0f, z=0.0f;
                    treeElement->Attribute("x", &x);
                    treeElement->Attribute("y", &y);
                    treeElement->Attribute("z", &z);
                    double treeHeight = 1.0f;
                    treeElement->Attribute("height", &treeHeight);
                    
                    const char* name = treeElement->Attribute("name");
                    StaticObject* tree = new StaticObject();
                    if( name )
                    {
                        tree->SetName(std::string(name));
                    }
                    float imgWidth = info.ImageWidth;
                    float imgHeight = info.ImageHeight;
                    float ratio = imgWidth / imgHeight;
                    tree->treeWidth = treeHeight * ratio;
                    tree->treeHeight = treeHeight;
                    tree->position = Vector2(x, z);
                    tree->texture = info.TextureID;
                    gameObjects.push_back(tree);
                    printf("Add Tree to scene:%s\n", tree->GetName().c_str());
                }
            }
        }
    }
    
    
    

#if 1
    class Sorter
    {
    public:
        bool operator()(const GameObject* obj0, const GameObject* obj1)
        {
            return obj0->position.y < obj1->position.y;
        }
    };
    std::sort(gameObjects.begin(), gameObjects.end(), Sorter());
#endif
}


Monster* BasicGameWorld::GetFreeStarMonster() const
{
    for(int j=0;j<starMonsters.size();++j)
    {
        if( ! starMonsters[j]->pathSpline )
            return starMonsters[j];
    }
    return NULL;
}


void BasicGameWorld::LoadPaths()
{
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    
    // star paths
    
    // normal paths
    
    {
        //std::string pathPath = root + "media/paths/path01.json";
        //TSpline* spline = animalsState->LoadSpline(pathPath);
        //splines.push_back(spline);
    }
    {
        //std::string pathPath = root + "media/paths/path02.json";
        //TSpline* spline = animalsState->LoadSpline(pathPath);
        //splines.push_back(spline);
    }
    {
        //std::string pathPath = root + "media/paths/path03.json";
        //TSpline* spline = animalsState->LoadSpline(pathPath);
        //splines.push_back(spline);
    }
    {
        //std::string pathPath = root + "media/paths/path04.json";
        //TSpline* spline = animalsState->LoadSpline(pathPath);
        //splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path01.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path02.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path03.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path04.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path05.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path06.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path07.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    {
        std::string pathPath = root + "media/paths/path08.json";
        TSpline* spline = animalsState->LoadSpline(pathPath);
        splines.push_back(spline);
    }
    
    
    printf("%d Paths Loaded!\n", (int)splines.size());
    
}


float BasicGameWorld::GetTotalPlayedTime() const
{
    return totalPlayedTime;
}

int BasicGameWorld::GetInZooAnimalCount() const
{
    int count = 0;
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        GameObject* gameObject = gameObjects[j];
        if( IsInside(gameObject->position) && !gameObject->IsIdle())
            ++ count;
    }
    return count;
}

int BasicGameWorld::GetLeavingZooAnimalCount() const
{
    int count = 0;
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        GameObject* gameObject = gameObjects[j];
        if( gameObject->isLeavingZoo && ! gameObject->IsIdle())
            ++ count;
    }
    return count;
}


#if defined(__APPLE__)
#pragma mark - Update
#endif

void BasicGameWorld::Restart()
{
    totalPlayedTime = 0.0f;
    zooManager->Restart();    
    for(int j=0;j<(int)starMonsters.size();++j)
    {
        starMonsters[j]->ChangeAction(NULL);
    }    
}

void BasicGameWorld::Update(double dt)
{
    totalPlayedTime += dt;
    
    zooManager->Update(dt);
    
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        gameObjects[j]->Update(dt);
    }
    for(int j=0;j<(int)starMonsters.size();++j)
    {
        starMonsters[j]->Update(dt);
    }
    
    
    // spider update
    for(int j=0;j<(int)spiders.size();++j)
    {
        spiders[j]->Update(dt);
    }
    
    
    for(int j=0;j<(int)bubbleEmitters.size();++j)
    {
        bubbleEmitters[j]->Update(dt);
    }
    
    
    if( willRenderCaustics )
    {
        int count = 32;
        float roundTime = 2.0f;
        float t = fmod(totalPlayedTime, roundTime);
        t = t / roundTime;
        int texIndex = (int)(count * t);
        assert( texIndex>=0 && texIndex < count );
        lightTexture = causticTextures[texIndex];
    }

    ResolveCollsion();
}

bool BasicGameWorld::IsInside(const Vector2& pos) const
{
    if( pos.x<minPt.x || pos.x>maxPt.x )
        return false;
    
    if( pos.y<minPt.y || pos.y>maxPt.y )
        return false;
    
    return true;
}

bool BasicGameWorld::EvalSpace(GameObject* g) const
{
    // TODO:
    return false;
}


void BasicGameWorld::ResolveCollsion()
{
#if 0
    int maxIteration = 1;
    for(int iter=0;iter<maxIteration;++iter)
    {
        
        std::vector<CollidedPair> collidedPairs;
        int count = (int)gameObjects.size();
        #if 0
        for(int j=0;j<count;++j)
        {
            Monster* m0 = dynamic_cast<Monster*>(gameObjects[j]);
            if( ! m0 )
                continue;
            
            if( m0->IsIdle())
                continue;
            
            for(int i=j+1;i<count;++i)
            {
                Monster* m1 = dynamic_cast<Monster*>(gameObjects[i]);
                if( m1 && !m1->IsIdle() )
                {
                    if( m0->GetOBB().Overlaps(m1->GetOBB()))
                    {
                        collidedPairs.push_back(CollidedPair(m0,m1));
                    }
                }
            }
        }
        #endif
        
        for(int j=0;j<count;++j)
        {
            GameObject* g0 = gameObjects[j];
            if( g0->IsIdle())
                continue;
            for(int i=1+j;i<count;++i)
            {
                GameObject* g1 = gameObjects[i];
                if( g1->IsIdle())
                    continue;
                
                //float distance = (g0->position - g1->position).length();
                //if( g0->radius + g1->radius < distance )
                //{
                //    collidedPairs.push_back(CollidedPair(g0, g1));
                //}
                if( g0->GetOBB().Overlaps(g1->GetOBB()))
                {
                    collidedPairs.push_back(CollidedPair(g0,g1));
                }
            }
        }
        
        if( collidedPairs.empty())
            break;
        
        //printf("%d Collided Pair found in Iteration:%d!\n", (int)collidedPairs.size(), iter);
        
        for(int j=0;j<collidedPairs.size();++j)
        {
            GameObject* object0 = collidedPairs[j].gameObject0;
            GameObject* object1 = collidedPairs[j].gameObject1;
            
            Vector2 ab = object0->position - object1->position;
            float dist = ab.normalise();
            
            
            float diff = dist - (object0->radius + object1->radius);
            if( diff < 0.0f )
            {
                //Vector2 posDiff = 0.5f * diff * a2b;
                if( dynamic_cast<Monster*>(object0))
                {
                    object0->position = object0->oldPosition;
                    object0->position += 20.f * ab;
                    object0->direction = (object0->direction + 1.0f*ab).normalisedCopy();
                }
                
                if( dynamic_cast<Monster*>(object1))
                {
                    object1->position = object1->oldPosition;                    
                    object1->position -= 20.f * ab;
                    object1->direction = (object1->direction - 1.0f*ab).normalisedCopy();
                }
                
                Monster* monster0 = dynamic_cast<Monster*>(object0);
                Monster* monster1 = dynamic_cast<Monster*>(object1);                
                if( monster0 && !monster0->isLeavingZoo )
                {
                    monster0->targetPosition = monster0->position + 200.0f* monster0->direction;
                }
                if( monster1 && !monster1->isLeavingZoo )
                {
                    monster1->targetPosition = monster1->position + 200.0f* monster1->direction;
                }
                
            }
        }
    }
#endif
}

#if defined(__APPLE__)
#pragma mark - Rendering
#endif


void BasicGameWorld::UpdateCameraFromVR()
{
    
    float x = playerPos.x;
    float y = playerPos.y;
    float z = playerPos.z;
    float angle = 0.0f;
    Vector3 pp = Vector3(x,y,z);
    Matrix4 headMatrix = GameStatics::GetInstance()->viewMatrix;
    
    Matrix3 rrr;
    headMatrix.extract3x3Matrix(rrr);
    Quaternion q;
    q.FromRotationMatrix(rrr);
    Quaternion qq;
    qq.FromAngleAxis(angle - Math::HALF_PI, Vector3::UNIT_Y);
    qq = Quaternion::IDENTITY;
    q = qq * q;
    
    Matrix3 rr;
    q.ToRotationMatrix(rr);
    
    viewMatrix = Matrix4(rr);
    viewMatrix._m[12] = -pp.dotProduct(rr.GetColumn(0));
    viewMatrix._m[13] = -pp.dotProduct(rr.GetColumn(1));
    viewMatrix._m[14] = -pp.dotProduct(rr.GetColumn(2));
    
    Vector3 lookDirection = - Vector3(viewMatrix.m[0][2], viewMatrix.m[1][2], viewMatrix.m[2][2]);
    Vector3 up = Vector3(viewMatrix.m[0][1], viewMatrix.m[1][1], viewMatrix.m[2][1]);
    mOrbitCamera->LookAt(playerPos, playerPos + lookDirection, up);
}



void BasicGameWorld::RenderSteerObject(SteerObject* s)
{
}

void BasicGameWorld::RenderMonster(Monster *monster)
{
#if 0
    if( ! monster)
        return;
    //if( monster->IsInZoo() )
    if( ! monster->IsIdle())
    {
        MainController* mainController = animalsState->mainController;
        assert( monster->GetAnimalType()>=0 && monster->GetAnimalType()<=3);
        FBXLoader* model = mainController->fbxModels[monster->GetAnimalType()];
        const Matrix4& transform = monster->GetTransform();
        float s = 26.0f;
        // Forest build artist request bigger animals
        if( SystemUtils::buildType==1)
        {
            s = 50.0f;
        }
        
        Matrix4 scale = Matrix4::getScale(Vector3(s));
        
        model->SetTexture(monster->texture);
        model->SetWorldTransform(scale * transform);
        
        //model->SetWorldTransform(scale);
        
        //fbxLoader->Render(monster->animalType);
        model->Render();
    }
    
#if 0
    const OBB2D& obb = monster->GetOBB();
    Vector3 pt0(obb.corner[0].x, 0.0f, obb.corner[0].y);
    Vector3 pt1(obb.corner[1].x, 0.0f, obb.corner[1].y);
    Vector3 pt2(obb.corner[2].x, 0.0f, obb.corner[2].y);
    Vector3 pt3(obb.corner[3].x, 0.0f, obb.corner[3].y);
    line3DRenderer->RenderLine(pt0, pt1);
    line3DRenderer->RenderLine(pt1, pt2);
    line3DRenderer->RenderLine(pt2, pt3);
    line3DRenderer->RenderLine(pt3, pt0);
#endif
#endif
}


Matrix4 BasicGameWorld::CalculateMonsterTransform(Monster* monster)
{
    MainController* mainController = animalsState->mainController;
    
    FBXLoader* model = mainController->fbxModels[monster->GetAnimalType()];
    Vector2 direction = monster->direction;
    Vector2 position = monster->position;
    float angle = atan2(-direction.y, -direction.x);
    Matrix4 rot = Matrix4::MakeYRotationMatrix(angle);
    Matrix4 transform = monster->GetTransform();
    Vector3 minPt, maxPt, center;
    model->GetBoundingBox(minPt, maxPt, center);
    float height = maxPt.y - minPt.y;
    float s = 26.0f;
    // Forest build artist request bigger animals
    if( SystemUtils::buildType==1)
    {
        s = 50.0f;
    }
    height = height * s;
    
    Matrix4 scale = Matrix4::getScale(Vector3(s));
    
    Matrix4 scaleRotate = scale * rot;
    
    monster->minPt = scaleRotate * minPt;
    monster->maxPt = scaleRotate * maxPt;
    
    Vector3 pos(position.x, 0.5f*height, position.y);
    
    #if 0
    if( willDrawTerrain )
    {
        float floorHeight = terrainRenderer->GetHeight(pos.x, pos.z);
        pos.y += floorHeight;
    }
    #endif
    
    monster->minPt += pos;
    monster->maxPt += pos;
    
    
    transform = Matrix4::getTrans(pos);
    return scaleRotate * transform.transpose();
}


void BasicGameWorld::Render(const Matrix4& v, const Matrix4& _projectionMatrix)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    TMathUtil::ExtractPlanes(frustumPlanes, _projectionMatrix, false);
    viewMatrix = v;
    inverseViewMatrix = viewMatrix.transpose();
    float w = Device::GetInstance()->GetLogicalWidth();
    float h = Device::GetInstance()->GetLogicalHeight();
    
    TFlatRenderer* flatRenderer = TFlatRenderer::GetInstance();
    flatRenderer->Reset();
    glDisable(GL_BLEND);
    
#if 1
    int buildType = SystemUtils::GetBuildType();
    if( ! willUseSkyBox )
    {
        flatRenderer->SetOrtho2D(0,w,h,0);
        flatRenderer->SetWorldTransform(Matrix4::IDENTITY);
        flatRenderer->BindTexture(backgroundTexture);
        
        TFlatRenderer::TVertexData pt0(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        TFlatRenderer::TVertexData pt1(   w, 0.0f, 0.0f, 1.0f, 1.0f);
        TFlatRenderer::TVertexData pt2(   w,    h, 0.0f, 1.0f, 0.0f);
        TFlatRenderer::TVertexData pt3(0.0f,    h, 0.0f, 0.0f, 0.0f);
        
        flatRenderer->RenderTriangle(pt0, pt1, pt2, TColor::White);
        flatRenderer->RenderTriangle(pt0, pt2, pt3, TColor::White);
        flatRenderer->Flush();
        glClear(GL_DEPTH_BUFFER_BIT);
    }
#endif

    
    
#if VR_BUILD
    UpdateCameraFromVR();
#endif
    
    float aspect = Device::GetInstance()->GetLogicalWidth() / Device::GetInstance()->GetLogicalHeight();
    Renderer* renderer = Renderer::GetInstance();
    
    
    float radius = mOrbitCamera->GetRadius();
    
    Vector3 eye = mOrbitCamera->GetPosition();
    Vector3 at = mOrbitCamera->mTarget;
    Vector3 up = mOrbitCamera->GetUp();
    
#if VR_BUILD
#else
    viewMatrix = Matrix4::MakeViewMatrix(eye, at, up);
#endif

    
    Matrix4 projectionMatrix = _projectionMatrix;
    
#if VR_BUILD
#else
    projectionMatrix = Matrix4::MakeProjectionMatrix(settings.fovy, aspect, settings.nearPlane, settings.farPlane);
#endif
    
    
#if 0
    printf("Radius:%f\n", mOrbitCamera->GetRadius());
    printf("Eye:[%f %f %f]\n", eye.x, eye.y, eye.z);
    printf("At :[%f %f %f]\n",  at.x,  at.y,  at.z);
    printf("Up :[%f %f %f]\n",  up.x,  up.y,  up.z);
#endif
    
    //renderer->SetProjection(settings.fovy, aspect, settings.nearPlane, settings.farPlane);
    //renderer->LookAt(eye, at, up);
    renderer->viewMatrix = viewMatrix;
    renderer->projectionMatrix = projectionMatrix;
    renderer->UpdateMatrix();
    renderer->Reset();
    
    
    TLine3DRenderer* line3DRenderer = TLine3DRenderer::GetInstance();
    line3DRenderer->Reset();
    //line3DRenderer->SetProjection(settings.fovy, aspect, settings.nearPlane, settings.farPlane);
    //line3DRenderer->LookAt(eye, at, up);
    
    line3DRenderer->mViewMatrix = viewMatrix;
    line3DRenderer->mProjectionMatrix = projectionMatrix;
    line3DRenderer->UpdateMatrix();
    
    
    
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    
    glDisable(GL_BLEND);
    
    
    
    glClear(GL_DEPTH_BUFFER_BIT);
    
    
    //line3DRenderer->SetColor(TColor::Red);
    
    
    
    
    renderer->Flush();
    renderer->Reset();

    
    //MainController* mainController = animalsState->mainController;
    
    
    frustumCulledCount = 0;
    renderedMonsterCount = 0;    
    glDisable(GL_BLEND);
    
    //
    if( viewMode==ViewMode::ViewMode3DDebug )
    {
        float r = 16000.0f;
        mOrbitCamera->LookAt(Vector3(0.0f, 0.0f, r), Vector3::ZERO, Vector3::UNIT_Y);
        float s = 18000.0f;
        skyBoxSize = s;
        settings.farPlane = 1.0f + sqrtf(4*s*s + 4*s*s);
    }
    
    
#if 1
    if( willUseSkyBox && skyBoxRenderer)
    {
        glEnable(GL_DEPTH_TEST);
        
        float s = skyBoxSize;
        skyBoxRenderer->viewMatrix = viewMatrix;
        skyBoxRenderer->projectionMatrix = projectionMatrix;
        Vector3 pp(playerPos.x, playerPos.y, playerPos.z);
        Matrix4 t = Matrix4::getTrans(pp).transpose();
        skyBoxRenderer->SetWorldTransform(Matrix4::getScale(s,s,s)*t);
        skyBoxRenderer->Render();
        glClear(GL_DEPTH_BUFFER_BIT);
    }
#endif


    //flatRenderer->SetProjection(settings.fovy, aspect, settings.nearPlane, settings.farPlane);
    //flatRenderer->LookAt(eye, at, up);
    flatRenderer->viewMatrix = viewMatrix;
    flatRenderer->projectionMatrix = projectionMatrix;
    flatRenderer->UpdateMatrix();
    
    flatRenderer->Reset();

    
    
    std::vector<Monster*> displayMonsters;
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(gameObjects[j]);
        if( monster && !monster->IsIdle() )
        {
            displayMonsters.push_back(monster);
        }
    }
    for(int j=0;j<(int)starMonsters.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(starMonsters[j]);
        if( monster && !monster->IsIdle() )
        {
            displayMonsters.push_back(monster);
        }
    }
    
    for(int j=0;j<(int)spiders.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(spiders[j]);
        if( monster )
        {
            displayMonsters.push_back(monster);
        }
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    
    for(int j=0;j<(int)displayMonsters.size();++j)
    {
        Monster* monster = displayMonsters[j];
        {
            MainController* mainController = animalsState->mainController;
            FBXLoader* model = mainController->fbxModels[monster->GetAnimalType()];
            
            Matrix4 worldTransform = CalculateMonsterTransform(monster);
            Vector3 minPt = monster->minPt;
            Vector3 maxPt = monster->maxPt;
            
            Vector3 minPoint = minPt;
            Vector3 maxPoint = maxPt;
            
            minPt = inverseViewMatrix * minPt;
            maxPt = inverseViewMatrix * maxPt;
            
            if( minPt.x > maxPt.x )
                std::swap(minPt.x, maxPt.x);
            if( minPt.y > maxPt.y )
                std::swap(minPt.y, maxPt.y);
            if( minPt.z > maxPt.z )
                std::swap(minPt.z, maxPt.z);
            AABB aabb(minPt, maxPt);
            
            
            
            if(  ! TMathUtil::IsBoxFullyOutsideFrustum(frustumPlanes, aabb))
            {
                monster->isCulled = false;
                model->SetTexture(monster->texture);
                model->SetWorldTransform(worldTransform);
                model->Render();
                ++ renderedMonsterCount;
                
            }
            else
            {
                monster->isCulled = true;
                ++ frustumCulledCount;
            }
            
            
            #if 0 // Render object BBox
            minPt = minPoint;
            maxPt = maxPoint;
            Vector3 pts[8];
            pts[0] = Vector3(minPt.x, minPt.y, minPt.z);
            pts[1] = Vector3(minPt.x, minPt.y, maxPt.z);
            pts[2] = Vector3(minPt.x, maxPt.y, maxPt.z);
            pts[3] = Vector3(minPt.x, maxPt.y, minPt.z);
            
            pts[4] = Vector3(maxPt.x, minPt.y, minPt.z);
            pts[5] = Vector3(maxPt.x, minPt.y, maxPt.z);
            pts[6] = Vector3(maxPt.x, maxPt.y, maxPt.z);
            pts[7] = Vector3(maxPt.x, maxPt.y, minPt.z);
            
            
            TLine3DRenderer* line3DRenderer = TLine3DRenderer::GetInstance();
            
            line3DRenderer->RenderLine(pts[0], pts[1]);
            line3DRenderer->RenderLine(pts[1], pts[2]);
            line3DRenderer->RenderLine(pts[2], pts[3]);
            line3DRenderer->RenderLine(pts[3], pts[0]);
            
            
            line3DRenderer->RenderLine(pts[4], pts[5]);
            line3DRenderer->RenderLine(pts[5], pts[6]);
            line3DRenderer->RenderLine(pts[6], pts[7]);
            line3DRenderer->RenderLine(pts[7], pts[4]);
            
            line3DRenderer->RenderLine(pts[0], pts[4]);
            line3DRenderer->RenderLine(pts[1], pts[5]);
            line3DRenderer->RenderLine(pts[2], pts[6]);
            line3DRenderer->RenderLine(pts[3], pts[7]);
            line3DRenderer->Flush();
            #endif

        }
    }
    
    
    
    //printf("Frustum Culled:%d:Rendered:%d\n", frustumCulledCount, renderedMonsterCount);
    
    

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    #if 1
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int j=0;j<(int)bubbleEmitters.size();++j)
    {
        bubbleEmitters[j]->Render(mOrbitCamera);
    }
    #endif
    
    
    
    
    
    
    
    
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for(int j=0;j<gameObjects.size();++j)
    {
        StaticObject* tree = dynamic_cast<StaticObject*>(gameObjects[j]);
        if( tree )
        {
            renderer->BindTexture(tree->texture);
            renderer->SetWorldTransform(Matrix4::IDENTITY);
            Vector3 pt = Vector3(tree->position.x, 0.0f, tree->position.y);
            float tw = 0.5f*tree->treeWidth;
            float th = tree->treeHeight;
            Vector3 pp = Vector3(tree->position.x, pt.y, tree->position.y);
            Vector3 U = tw * Vector3::UNIT_X;
            Vector3 V = th * mOrbitCamera->GetUp();
            Vector3 pp0 = pp - U;
            Vector3 pp1 = pp + U;
            Vector3 pp2 = pp + U + V;
            Vector3 pp3 = pp - U + V;
            
            float u0 = 0.001f;
            float u1 = 0.999f;
            float v0 = 0.001f;
            float v1 = 0.999f;
            Renderer::TVertexData pt0(pp0.x, pp0.y, pp0.z, u0, v0);
            Renderer::TVertexData pt1(pp1.x, pp1.y, pp1.z, u1, v0);
            Renderer::TVertexData pt2(pp2.x, pp2.y, pp2.z, u1, v1);
            Renderer::TVertexData pt3(pp3.x, pp3.y, pp3.z, u0, v1);
            
            renderer->RenderTriangle(pt0, pt1, pt2);
            renderer->RenderTriangle(pt0, pt2, pt3);
            renderer->Flush();
        }
    }
    #if 0
    for(int j=0;j<animalsState->trees.size();++j)
    {
        TreeObject* tree = &animalsState->trees[j];
        int count = 32;
        float angle = 0.0f;
        float angleInc = Math::TWO_PI / count;
        line3DRenderer->SetColor(TColor::Red);
        for(int j=0;j<count;++j)
        {
            float x0 = tree->radius * cos(angle) + tree->position.x;
            float y0 = tree->radius * sin(angle) + tree->position.z;
            angle += angleInc;
            float x1 = tree->radius * cos(angle) + tree->position.x;
            float y1 = tree->radius * sin(angle) + tree->position.z;
            line3DRenderer->RenderLine(Vector3(x0, 0.01f, y0), Vector3(x1, 0.01f, y1));
        }
        line3DRenderer->Flush();

    }
    #endif
    
    
    #if 0
    line3DRenderer->SetColor(TColor::Red);
    if( workingMode == WorkingModeEditMode)
    {
        for(int j=1;j<(int)controlPoints.size();++j)
        {
            Vector3 pt0 = controlPoints[j-1];
            Vector3 pt1 = controlPoints[j];
            line3DRenderer->RenderLine(pt0, pt1);
        }
        line3DRenderer->Flush();
    }
    #endif
    
    
    glDisable(GL_DEPTH_TEST);
    
    
    
    if( willRenderCaustics )
    {
        glEnable(GL_BLEND);
#if 1
#if TARGET_OS_OSX
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
#else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#endif
        
        
    flatRenderer->SetWorldTransform(Matrix4::IDENTITY);
    flatRenderer->SetOrtho2D(0.0f, w, h, 0.0f);
    flatRenderer->BindTexture(lightTexture);
    flatRenderer->SetWrapMode(GL_REPEAT);
    
    Vector3 pp0(0.0f, 0.0f, 0.0f);
    Vector3 pp1(   w, 0.0f, 0.0f);
    Vector3 pp2(   w,    h, 0.0f);
    Vector3 pp3(0.0f,    h, 0.0f);
    float t = fmod(0.04f*totalPlayedTime, 1.0f);
    float u0 = t;
    float u1 = 1.0f + t;
    float v0 = 0.0f;
    float v1 = 1.0f;
    TFlatRenderer::TVertexData pt0(pp0.x, pp0.y, pp0.z, u0, v0);
    TFlatRenderer::TVertexData pt1(pp1.x, pp1.y, pp1.z, u1, v0);
    TFlatRenderer::TVertexData pt2(pp2.x, pp2.y, pp2.z, u1, v1);
    TFlatRenderer::TVertexData pt3(pp3.x, pp3.y, pp3.z, u0, v1);
    
    TColor color = TColor(1.0f, 1.0f, 1.0f, 0.14f);
    
    #if TARGET_OS_OSX
    {
    GLint swizzleMask[] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    #endif
    
    flatRenderer->RenderTriangle(pt0, pt1, pt2, color);
    flatRenderer->RenderTriangle(pt0, pt2, pt3, color);
    flatRenderer->Flush();
    #if TARGET_OS_OSX
    {
    GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    #endif
        
    flatRenderer->SetWrapMode(GL_CLAMP_TO_EDGE);
        
        
#endif
    }

    
    

    
    
#if 1
    if( drawPath )
    {
        glDisable(GL_DEPTH_TEST);
        line3DRenderer->SetColor(TColor::Red);
        //glLineWidth(6.0f);
        for(int j=0;j<splines.size();++j)
        {
            RenderSpline(splines[j]);
        }
        line3DRenderer->SetColor(TColor::Green);
        for(int j=0;j<starSplines.size();++j)
        {
            RenderSpline(starSplines[j]);
        }

    }
#endif
    
    
    
#if 0 // Debug Display
    
    
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
    TFlatRenderer::RenderRect(0.0f, 0.0f, 512, 512, lightTexture);
    
#endif
}

void BasicGameWorld::RenderSpline(TSpline* s)
{
    const std::vector<Vector3> & cps = s->GetControlPoints();
    if( cps.empty())
        return;
    
    TLine3DRenderer* line3DRenderer = TLine3DRenderer::GetInstance();
    //Vector3 center = cps[0];
    for(int j=1;j<(int)cps.size();++j)
    {
        //center += cps[j];
        Vector3 pt0 = cps[j-1];
        Vector3 pt1 = cps[j];
        line3DRenderer->RenderLine(pt0, pt1);
    }
    line3DRenderer->Flush();
    
    //MainController* mainController = animalsState->mainController;
    //center /= (float)cps.size();
    

    
    
}

void BasicGameWorld::RenderMonsterMessage(Monster* monster)
{
    
    if( (! monster) || monster->IsIdle() )
        return;
    
    if( monster->isCulled )
        return;
    
    MainController* mainController = animalsState->mainController;
    TFlatRenderer* flatRenderer = TFlatRenderer::GetInstance();
    const TColor& nameTagBackgroundColor = animalsState->settings.nameTagBackgroundColor;
    const TColor& fontColor = animalsState->settings.nameTagTextColor;
    
    Matrix4 t = monster->GetTransform();
    Vector3 translate(t[3][0], t[3][1], t[3][2]);
    translate += Vector3(0.0f, 250.0f, 0.0f);
    
    Vector2 pt = animalsState->ToScreenPosition(translate);
    pt.y += animalsState->settings.nameTagOffset;
    float hw = 60.0f;
    float hh = 20.0f;
    Vector2 pt0(pt.x - hw, pt.y - hh);
    Vector2 pt1(pt.x + hw, pt.y - hh);
    Vector2 pt2(pt.x + hw, pt.y + hh);
    Vector2 pt3(pt.x - hw, pt.y + hh);
    //flatRenderer->RenderLine(pt0, pt1, thickness, TColor::Orange);
    //flatRenderer->RenderLine(pt1, pt2, thickness, TColor::Orange);
    //flatRenderer->RenderLine(pt2, pt3, thickness, TColor::Orange);
    //flatRenderer->RenderLine(pt3, pt0, thickness, TColor::Orange);
    
    flatRenderer->BindTexture(mainController->whiteTexture);
    
    TFlatRenderer::TVertexData p0(pt0.x, pt0.y, 0.0f, 0.0f, 0.0f);
    TFlatRenderer::TVertexData p1(pt1.x, pt1.y, 0.0f, 0.0f, 0.0f);
    TFlatRenderer::TVertexData p2(pt2.x, pt2.y, 0.0f, 0.0f, 0.0f);
    TFlatRenderer::TVertexData p3(pt3.x, pt3.y, 0.0f, 0.0f, 0.0f);
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    flatRenderer->RenderTriangle(p0, p1, p2, nameTagBackgroundColor);
    flatRenderer->RenderTriangle(p0, p2, p3, nameTagBackgroundColor);
    flatRenderer->Flush();
    //mainController->RenderText(pt.x, pt.y, monster->GetMessage(), 14, fontColor);
    mainController->RenderText(pt.x, pt.y, monster->GetDisplayName(), 14, fontColor);
}


void BasicGameWorld::RenderMessages()
{
    MainController* mainController = animalsState->mainController;
    
    TFlatRenderer* flatRenderer = TFlatRenderer::GetInstance();
#if 1
    flatRenderer->BindTexture(mainController->whiteTexture);
    //const float thickness = 1.0f;
    for(int j=0;j<(int)gameObjects.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(gameObjects[j]);
        if( monster )
            RenderMonsterMessage(monster);
    }
#endif
    
    for(int j=0;j<(int)starMonsters.size();++j)
    {
        RenderMonsterMessage(starMonsters[j]);
    }
    
    for(int j=0;j<(int)spiders.size();++j)
    {
        Monster* monster = dynamic_cast<Monster*>(spiders[j]);
        if( monster )
            RenderMonsterMessage(monster);
    }
}

void BasicGameWorld::OnTouchDown(const std::vector<TTouchControl>& touches)
{
    
    float x = touches[0].x;
    float y = touches[0].y;
    
    if( ! settings.isFixedCamera )
    {
        mOrbitCamera->InjectMouseDown(x, y);
    }
    
    
#if 1
    if( x < 50.0f && y < 50.0f )
    {
        mOrbitCamera->LookAt(settings.defaultCameraEye, settings.defaultCameraAt, settings.defaultCameraUp);
#if 0
        if( workingMode == WorkingModePlayMode )
        {
            workingMode = WorkingModeEditMode;
        }
        else
        {
            workingMode = WorkingModePlayMode;
        }
#endif
    }
#endif
    
        
    
    
    
#if 0
    if( x > 900.0f && y < 50.0f )
    {
        BasicGameWorld->Restart();
#if 0
        if( workingMode == WorkingModeEditMode)
        {
            controlPoints.clear();
            PrintControlPoints();
        }
#endif
    }
#endif
#if 0
    if( x < 50.0f && y > 700.0f )
        BasicGameWorld->drawPath = ! BasicGameWorld->drawPath;
#endif
    
#if 0
    if( x > 900.0f && y > 700.0f )
        willRenderFloor = ! willRenderFloor;
#endif
    
    
    
    Ray ray = CalPickRay(x, y);
    
    
    float u = 0, v = 0;
    float t = TMathUtil::GetRayToInfinityPlaneDistance(
                                                       ray
                                                       , Vector3::ZERO
                                                       , Vector3::UNIT_X
                                                       , Vector3::UNIT_Z
                                                       ,	&u
                                                       ,	&v
                                                       ,true
                                                       );
    
    
    if( workingMode == WorkingModeEditMode)
    {
        if( t > 0.0f )
        {
            //Vector3 pp = ray.getPoint(t);
            //controlPoints.push_back(pp);
            
            //PrintControlPoints();
        }
    }

    

}

void BasicGameWorld::OnTouchMoved(const std::vector<TTouchControl>& touches)
{
    const TTouchControl* touch = &touches[0];
    if( ! settings.isFixedCamera )
        mOrbitCamera->InjectMouseMove(touch->x, touch->y);

}

void BasicGameWorld::OnTouchUp(const std::vector<TTouchControl>& touches)
{
    const TTouchControl* touch = &touches[0];
    if( ! settings.isFixedCamera )
        mOrbitCamera->InjectMouseUp(touch->x, touch->y);
}

void BasicGameWorld::OnPinched(float scale, float x, float y, float dx, float dy)
{
    #if 1
    if( settings.isFixedCamera )
        return;

    if( scale==0.0f)
        return;

    const float minDist = 1.0f;
    const float maxDist = 7000.0f;

    float r = mOrbitCamera->GetRadius();
    r = r / scale;
    r = r < minDist ? minDist : r;
    r = r > maxDist ? maxDist : r;

    mOrbitCamera->SetRadius(r);
#endif
}

void BasicGameWorld::OnScrollWheel(float delta)
{
    Vector3 eye = mOrbitCamera->GetPosition();
    Vector3 at = mOrbitCamera->mTarget;
    Vector3 up = mOrbitCamera->GetUp();
    Vector3 N = (at - eye).normalisedCopy();
    float f = 16.0f;
    Vector3 inc = f * delta * N;
    eye += inc;
    at += inc;
    mOrbitCamera->LookAt(eye, at, up);
    
#if 0
    printf("Radius:%f\n", mOrbitCamera->GetRadius());
    printf("Eye:[%f %f %f]\n", eye.x, eye.y, eye.z);
    printf("At :[%f %f %f]\n",  at.x,  at.y,  at.z);
    printf("Up :[%f %f %f]\n",  up.x,  up.y,  up.z);
#endif
}


void BasicGameWorld::OnResize(float width, float height)
{
    FixCameraPositionFromScreenResolution();
}


void BasicGameWorld::FixCameraPositionFromScreenResolution()
{
#if 1
    if( SystemUtils::GetBuildType()==1)
    {
        ViewParam v0 = initialViewParam;
        ViewParam v1 = ViewParam::Make16To9ViewParam();
        
        float r0 = 4.0f / 3.0f;
        float r1 = 16.0f / 9.0f;
        float r = Device::GetInstance()->GetLogicalWidth() / Device::GetInstance()->GetLogicalHeight();
        float t = (r-r0) / (r1-r0);
        t = t < 0.0f ? 0.0f : t;
        t = t > 1.0f ? 1.0f : t;
        Vector3 eye = v0.eye + t * (v1.eye - v0.eye);
        Vector3  at = v0.at  + t * (v1.at - v0.at);
        Vector3  up = v0.up  + t * (v1.up - v0.up);
        mOrbitCamera->LookAt(eye, at, up);
    }
#endif
}



Ray BasicGameWorld::CalPickRay(int mx, int my)
{
    int width = 1024;//TGraphicsDevice::GetInstance().GetWidth();
    int height = 768;//TGraphicsDevice::GetInstance().GetHeight();
    
    Renderer * renderer = Renderer::GetInstance();
    Vector3 eye = mOrbitCamera->GetPosition();
    Vector3 at = mOrbitCamera->mTarget;
    Vector3 up = mOrbitCamera->GetUp();
    
    
    renderer->LookAt(eye, at, up);
    renderer->SetProjection(settings.fovy,  (float)width / (float)height, settings.nearPlane, settings.farPlane );
    
    Vector3 pt1 = renderer->UnProject(Vector3((float)mx, (float)my,1.0f), width, height, 1000.0f);
    Vector3 pt0 = renderer->UnProject(Vector3((float)mx, (float)my,0.0f), width, height, 1.0f);
    
    Vector3 dir = (pt1 - pt0).normalisedCopy();
    Ray ray(pt0,dir);
    return ray;
}


