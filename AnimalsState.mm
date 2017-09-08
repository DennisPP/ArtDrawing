#include "AnimalsState.h"



#include "Core/GremlinsFramework.h"
#include "Core/Device.h"
#include "Core/TouchControl.h"

#include "Camera/OrbitCamera.h"

#include "Graphics/Renderer.h"
#include "Graphics/FlatRenderer.h"

#include "Graphics/TextureManager.h"

#include "Graphics/Line3DRenderer.h"


#include "fbxsdk.h"

#include "FBXLoader.h"
#include "OBJModel.h"

#include "Monster.h"

#include "MathUtil.h"

#include "MainController.h"
#include "OBB2D.h"

//#include "GameWorld.h"
#include "SpriteAnimation/Sprite.h"

#include "json.hpp"

#include "SystemUtils.h"

#include "tinyxml.h"


#include "Font/FontEngine.h"
#include "GameStatics.h"

#include "TerrainRenderer.h"

#import "AnimalStateImp.h"

#include "BasicGameWorld.h"

using namespace FontEngine;


AnimalStateImp* animalStateImp = nil;

ViewParam::ViewParam()
{
    eye = Vector3(0.0f, 0.0f, 10.0f);
    at = Vector3::ZERO;
    up = Vector3::UNIT_Y;
}


ViewParam::ViewParam(const Vector3& _eye, const Vector3& _at, const Vector3& _up)
:   eye(_eye)
,   at(_at)
,   up(_up)
{
}

ViewParam ViewParam::Make16To9ViewParam()
{
    ViewParam p;
    p.eye = Vector3(0.000000f, 2410.204590f, 4673.080078f);
    p.at  = Vector3(0.000000f, -725.914307f, -442.066956f);
    p.up  = Vector3(0.000000f,    0.852525f,   -0.522687f);
    return p;
}

StudentInfo::StudentInfo()
{
}

StudentInfo::StudentInfo(const std::string& _className, const std::string& _name)
:   className(_className)
,   name(_name)
{    
}

AnimalsState::Settings::Settings()
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


struct FBOVertex
{
    float position[3];
    float texCoord[2];
    float normal[3];
};

struct FBOLump
{
    unsigned int offset;
    unsigned int length;
};

struct FBOHeader
{
    char magic[4];
    unsigned int version;
    FBOLump      lump[4]; // directory of the lumps
};


AnimalsState::AnimalsState(MainController* controller)
:  GameState(controller)
{
}


bool AnimalsState::Create()
{
    
    playerPos = Vector3(0.0f, 100.0f, 0.0f);
    playerShouldMove = false;
    
    workingMode = WorkingModePlayMode;
    
    viewMode = ViewMode::ViewMode2DDisplay;
    
    
#if TARGET_OS_OSX
    //viewMode = ViewMode::ViewMode3DDebug;
#endif
    
    totalPlayedTime = 0.0f;
    
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    
    
    activityLogoSprite = TSprite::SpriteFromImage((root+"media/2x/logo_part2017@2x~ipad.png").c_str());
  
    
    
    
    
    
    
    animalTextures[0] = TTextureManager::GetInstance().GetTexture(mainController->drawItems[0].originalTexturePath.c_str()).TextureID;
    animalTextures[1] = TTextureManager::GetInstance().GetTexture(mainController->drawItems[1].originalTexturePath.c_str()).TextureID;
    animalTextures[2] = TTextureManager::GetInstance().GetTexture(mainController->drawItems[2].originalTexturePath.c_str()).TextureID;
    animalTextures[3] = TTextureManager::GetInstance().GetTexture(mainController->drawItems[3].originalTexturePath.c_str()).TextureID;
    
    
       
    
    
    
    
    for(int j=0;j<4 &&j<(int)mainController->fbxModels.size();++j)
    {
        mainController->fbxModels[j]->SetTexture(animalTextures[j]);
    }    
    
#if 0
    printf("Joint Tree Size:%d\n", (int)fbxModels[0]->joints.size());
    if( !fbxLoader->joints.empty())
    {
        fbxLoader->joints[0]->Dump();
    }
#endif
    
    TFontEngine::GetInstance()->Cache(L"0123456789", settings.fontSize);
    
    if( settings.useOfflineData )
    {
        LoadStudentInfo();
    }
    
    
    
    
    willRenderFloor = false;
    
#if VR_BUILD
    willRenderFloor = false;
    floorTexture = TTextureManager::GetInstance().GetTexture((root + "media/textures/skybox/skybox_neg_y.png").c_str()).TextureID;
#endif

    
    skyBoxRenderer = NULL;
    willUseSkyBox = viewMode == ViewMode::ViewMode3DDebug;
#if VR_BUILD
    willUseSkyBox = true;
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

    
#if VR_BUILD
    willDrawTerrain = true;
#else
    willDrawTerrain = viewMode == ViewMode::ViewMode3DDebug;
#endif
    terrainRenderer = NULL;
    if( willDrawTerrain )
    {
        terrainTexture = TTextureManager::GetInstance().GetTexture((root + "media/terrain.png").c_str()).TextureID;
        terrainRenderer = new TerrainRenderer();
        terrainRenderer->Create();
        terrainRenderer->BindTexture(terrainTexture);
    }
    
    
    //
       
    
    animalStateImp = [[AnimalStateImp alloc] initWithState:(AnimalsState*)this];
    
    
    
    initialViewParam.eye = settings.defaultCameraEye;
    initialViewParam.at  = settings.defaultCameraAt;
    initialViewParam.up  = settings.defaultCameraUp;
    
    
    // 2. Create game objects from student information
    CreateGameObjects();
    
    
    return true;
}



void AnimalsState::CreateGameObjects()
{
    
    gameWorld = new BasicGameWorld(this);
    gameWorld->Create();
    
  
}

void AnimalsState::LoadStudentInfo()
{
    // 1. use google doc to export the excel file to csv, google doc can handle UTF8 string
    // 2. use http://www.convertcsv.com/csv-to-xml.htm to convert to xml
    const std::string & root = TGremlinsFramework::GetInstance()->GetAssetRoot();
    const std::string path = (root + "media/students.xml");
    TiXmlDocument doc;
    doc.LoadFile(path.c_str());
    
    TiXmlElement * rootElement = doc.FirstChildElement("root");
    assert( rootElement );
    
    
    int failCount = 0;
    TiXmlElement* row = rootElement->FirstChildElement("row");
    std::vector<std::string> failedTextures;
    for(;row;row=row->NextSiblingElement("row"))
    {
        StudentInfo s;
        TiXmlElement* classElement = row->FirstChildElement("Class");
        if( classElement )
        {
            s.className = std::string(classElement->GetText());
            printf("Class:%s\n", classElement->GetText());
        }
        TiXmlElement* nameElement = row->FirstChildElement("StudentName");
        if( nameElement )
        {
            printf("Name:%s\n", nameElement->GetText());
            s.name = std::string(nameElement->GetText());
        }
        TiXmlElement* texFileElement = row->FirstChildElement("WorksheetFilename");
        bool ok = true;
        if( texFileElement )
        {
            printf("WorksheetFilename:%s\n", texFileElement->GetText());
            std::string ss = std::string(texFileElement->GetText());
            std::string dir = "lion";
            if( ss.find("lion") != std::string::npos)
            {
                dir = "lion";
                s.animalType = 0;
            }
            else if( ss.find("rhino") != std::string::npos)
            {
                s.animalType = 1;
                dir = "rhino";
            }
            if( ss.find("giraffe") != std::string::npos)
            {
                s.animalType = 2;
                dir = "giraffe";
            }
            s.texturePath = root + "media/textures/" + dir + "/" + ss + ".png";
            printf("Texture Path:%s\n", s.texturePath.c_str());
            
            if( ! SystemUtils::IsFileExist(s.texturePath))
            {
                ok = false;
                ++ failCount;
                failedTextures.push_back(s.texturePath);
                printf("TEXTURE FILE NOT FOUND:%s\n", s.texturePath.c_str());
            }
            else
            {
                #if 0
                unsigned int texture = TTextureManager::GetInstance().GetTexture(s.texturePath.c_str()).TextureID;
                s.texture = texture;
                #endif
                
                // 
                #if 1
                s.animalType = rand()%4;
                const DrawItem& drawItem = mainController->drawItems[s.animalType];
                unsigned int texture = TTextureManager::GetInstance().GetTexture(drawItem.originalTexturePath.c_str()).TextureID;
                s.texture = texture;
                #endif
            }
        }
        if( ok ) 
        {
            s.displayName = s.className + " " + s.name;
            TFontEngine* fontEngine = TFontEngine::GetInstance();
            std::wstring ws = fontEngine->StringToWString(s.displayName);
            fontEngine->Cache(ws, settings.fontSize);
            studentInfos.push_back(s);
        }
    }
    printf("%d Student Loaded Successfully!\n", (int)studentInfos.size());
    printf("%d Student Loaded FAILED!\n", failCount);
    for(int j=0;j<(int)failedTextures.size();++j)
    {
        printf("TEXTURE FILE NOT FOUND:%d:%s\n", j, failedTextures[j].c_str());
    }
}


void AnimalsState::Reset()
{
    totalPlayedTime = 0.0f;
}

void AnimalsState::Destroy()
{
    //SAFE_DELETE(skyBox);
}

void AnimalsState::ResetToDefaultCameraSettings()
{
    //mOrbitCamera->LookAt(settings.defaultCameraEye, settings.defaultCameraAt, settings.defaultCameraUp);
}

std::string AnimalsState::LoadFile(const std::string &path)
{
    FILE *fp ;
    char* buffer;
    const char* pFilename = path.c_str();
    
    
    fp = fopen(pFilename, "rt");
    if( ! fp )
    {
        printf("Failed loading:%s\n", pFilename);
    }
    assert( fp && pFilename );
    if( ! fp )
        return "";
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    buffer = (char*) malloc(size+1);
    
    memset( buffer, 0, size+1);
    
    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, size, fp);
    
    printf("%s\n", buffer);
    std::string s(buffer);
    free(buffer);
    fclose(fp);
    return s;
}

TSpline* AnimalsState::LoadSpline(const std::string &filePath)
{
    using namespace nlohmann;
    std::string s = LoadFile(filePath);
    json js = json::parse(s);
    if( ! js.is_object())
        return NULL;
    
    TSpline* spline = new TSpline();
    json path = js["path"];
    if( path.is_array())
    {
        for(int j=0;j<(int)path.size();++j)
        {
            json pt = path[j];
            float x = pt["x"];
            float y = pt["y"];
            spline->AddControlPoint(Vector3(x,0.0f,y));
        }
    }
    spline->Build();
    return spline;
}


float AnimalsState::GetTotalPlayedTime() const
{
    return totalPlayedTime;
}

void AnimalsState::UpdateCameraFromVR()
{
    
}




#if defined(__APPLE__)
#pragma mark - Rendering
#endif

void AnimalsState::DumpNode(FbxNode* node, int depth)
{
    if( !node )
        return;
    
    const char space = ' ';
    for(int j=0;j<depth;++j)
    {
        printf("%c", space);
    }
    printf("%s\n", node->GetName());
    
    
    FbxMesh *mesh = node->GetMesh();
    if(mesh)
    {
        
        int numVerts = mesh->GetControlPointsCount();
        for (int j = 0; j<numVerts; j++)
        {
            FbxVector4 v = mesh->GetControlPointAt(j);
            //printf("V[%d]:%f %f %f %f\n", j, v.mData[0], v.mData[1], v.mData[2], v.mData[3]);
        }
    }
    for(int i = 0; i < node->GetChildCount(); i++)
    {
        DumpNode(node->GetChild(i), 1+depth);
    }
}
#pragma mark - Update

void AnimalsState::UpdatePlayer(double dt)
{
    Matrix4 headMatrix = GameStatics::GetInstance()->viewMatrix;
    Vector3 lookDirection = - Vector3(headMatrix.m[0][2], headMatrix.m[1][2], headMatrix.m[2][2]);
    lookDirection.y = 0.0f;
    lookDirection.normalise();
    float moveSize = 1000.0f;
    playerPos += dt * moveSize * lookDirection;
    
    if( willDrawTerrain )
    {
        playerPos.y = 100.0f + terrainRenderer->GetHeight(playerPos.x, playerPos.z);
    }
}


void AnimalsState::Update(double dt)
{    
    totalPlayedTime += dt;
    
    if( playerShouldMove )
        UpdatePlayer(dt);
    
    for(int j=0;j<(int)mainController->fbxModels.size();++j)
    {
        mainController->fbxModels[j]->Update(dt);
    }
    
    
    gameWorld->Update(dt);
    
    
    
    if( animalStateImp )
    {
        [animalStateImp update:dt];
    }
}

void AnimalsState::OnVREventFired()
{
    printf("%s\n", __FUNCTION__);
    
    playerShouldMove = ! playerShouldMove;
}

void AnimalsState::RenderFloor()
{
    int count = 40;
    float totalSize = 40000.0f;
    float x0 = -totalSize * 0.5f;
    float x1 =  totalSize * 0.5f;
    float z0 = -totalSize * 0.5f;
    float z1 =  totalSize * 0.5f;
    
    
    float y = 0.0f;
    const bool renderCheckerFloor = false;
    if( renderCheckerFloor )
    {
        #if 1
        TFlatRenderer* renderer = TFlatRenderer::GetInstance();
        Vector3 pp = playerPos;
        pp.y = 0.0f;
        //Matrix4 t = Matrix4::getTrans(pp).transpose();
        //renderer->SetWorldTransform(t);
        
        renderer->BindTexture(floorTexture);
        
        TFlatRenderer::TVertexData pt0(x0, y, z0, 0.0f, 0.0f);
        TFlatRenderer::TVertexData pt1(x1, y, z0, 1.0f, 0.0f);
        TFlatRenderer::TVertexData pt2(x1, y, z1, 1.0f, 1.0f);
        TFlatRenderer::TVertexData pt3(x0, y, z1, 0.0f, 1.0f);
        
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        renderer->RenderTriangle(pt0, pt1, pt2);
        renderer->RenderTriangle(pt0, pt2, pt3);
        renderer->Flush();
        #endif
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        
        float x = x0;
        float inc = totalSize / (float)count;
        
        TLine3DRenderer* line3DRenderer = TLine3DRenderer::GetInstance();
        //glLineWidth(2.0f);
        line3DRenderer->SetColor(TColor(0.4f, 0.4f, 0.4f, 1.0f));
        
        for(int j=0;j<=count;++j)
        {
            Vector3 pt0(x, 0.0f, z0);
            Vector3 pt1(x, 0.0f, z1);
            line3DRenderer->RenderLine(pt0, pt1);
            x += inc;
        }
        float z = z0;
        for(int j=0;j<=count;++j)
        {
            Vector3 pt0(x0, 0.0f, z);
            Vector3 pt1(x1, 0.0f, z);
            line3DRenderer->RenderLine(pt0, pt1);
            z += inc;
        }
        line3DRenderer->Flush();
        
        glEnable(GL_DEPTH_TEST);
    }
}

Vector2 AnimalsState::ToScreenPosition(const Vector3& worldPt) const
{
    Renderer* renderer = Renderer::GetInstance();
    Vector3 p = worldPt;
    
    
    float w = Device::GetInstance()->GetLogicalWidth();
    float h = Device::GetInstance()->GetLogicalHeight();    
    
    p = renderer->WorldToClipSpace(p);
    p.x += 1.0f;
    p.y += 1.0f;
    p.y = 2.0f - p.y;
    p.x *= 0.5f*w;
    p.y *= 0.5f*h;
    return Vector2(p.x, p.y);
}

void AnimalsState::PrintControlPoints()
{
    
    printf("{\n");
    printf("\"path\" : [\n");
    int count = (int)controlPoints.size();
    for(int j=0;j<count;++j)
    {
        printf("{ \"x\":%f, \"y\":%f }\n", controlPoints[j].x, controlPoints[j].z);
        if( j<count-1)
            printf(",");
    }
    printf("]\n");
    printf("}\n");
}


void AnimalsState::Render()
{
    
    
    float w = Device::GetInstance()->GetLogicalWidth();
    float h = Device::GetInstance()->GetLogicalHeight();
    float aspect = w / h;
    TFlatRenderer* flatRenderer = TFlatRenderer::GetInstance();
     
    
    
    if( gameWorld )
    {
        const Matrix4& viewMatric = GameStatics::GetInstance()->viewMatrix;
        const Matrix4& projectionMatrix = GameStatics::GetInstance()->projectionMatrix;
        gameWorld->Render(viewMatrix, projectionMatrix);
        
        flatRenderer->SetOrtho2D(0.0f, w, h, 0.0f);
        gameWorld->RenderMessages();
    }
    
    
}


#if 0
void AnimalsState::RenderOld()
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    
#if 0
    GLint colorBits[4];
    glGetIntegerv(GL_RED_BITS, &colorBits[0]);
    glGetIntegerv(GL_GREEN_BITS, &colorBits[1]);
    glGetIntegerv(GL_BLUE_BITS, &colorBits[2]);
    glGetIntegerv(GL_ALPHA_BITS, &colorBits[3]);
    printf("Color Depth:[%d %d %d %d]\n", colorBits[0], colorBits[1], colorBits[2], colorBits[3]);
#endif
    
    
    
    //glLineWidth(15.0f);
    
    
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
    
    
    Matrix4 projectionMatrix = GameStatics::GetInstance()->projectionMatrix;
    
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
    
    
    TFlatRenderer* flatRenderer = TFlatRenderer::GetInstance();
    //flatRenderer->SetProjection(settings.fovy, aspect, settings.nearPlane, settings.farPlane);
    //flatRenderer->LookAt(eye, at, up);
    flatRenderer->viewMatrix = viewMatrix;
    flatRenderer->projectionMatrix = projectionMatrix;
    flatRenderer->UpdateMatrix();
    
    flatRenderer->Reset();
    
    
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    
    glDisable(GL_BLEND);


#if 1
    if( ! willUseSkyBox )
    {
        flatRenderer->SetOrtho2D(0,1024,0,768);
        flatRenderer->SetWorldTransform(Matrix4::IDENTITY);
        float w = 1024.0f;
        float h = 768.0f;
        
        //flatRenderer->BindTexture(mainController->whiteTexture);
        
        flatRenderer->BindTexture(backgroundTexture);
        
        TFlatRenderer::TVertexData pt0(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        TFlatRenderer::TVertexData pt1(   w, 0.0f, 0.0f, 1.0f, 0.0f);
        TFlatRenderer::TVertexData pt2(   w,    h, 0.0f, 1.0f, 1.0f);
        TFlatRenderer::TVertexData pt3(0.0f,    h, 0.0f, 0.0f, 1.0f);
        
        flatRenderer->RenderTriangle(pt0, pt1, pt2, TColor::White);
        flatRenderer->RenderTriangle(pt0, pt2, pt3, TColor::White);
        flatRenderer->Flush();
    }
    
#endif

    
    glClear(GL_DEPTH_BUFFER_BIT);
    
    
    //line3DRenderer->SetColor(TColor::Red);
    
    
    
    renderer->BindTexture(whiteTexture);
   
   
    renderer->Flush();
    renderer->Reset();
    
    
#if 1
    if( willUseSkyBox && skyBoxRenderer)
    {
        glEnable(GL_DEPTH_TEST);
        
        float s = settings.skyBoxSize;
        skyBoxRenderer->viewMatrix = viewMatrix;
        skyBoxRenderer->projectionMatrix = projectionMatrix;
        Vector3 pp(playerPos.x, playerPos.y, playerPos.z);
        Matrix4 t = Matrix4::getTrans(pp).transpose();
        skyBoxRenderer->SetWorldTransform(Matrix4::getScale(s,s,s)*t);
        skyBoxRenderer->Render();
        glClear(GL_DEPTH_BUFFER_BIT);
    }
#endif
    
    //skyBox->Render();
    
    if( willRenderFloor)
        RenderFloor();
    
    
    glEnable(GL_DEPTH_TEST);
    
    
    if( willDrawTerrain && terrainRenderer )
    {
        terrainRenderer->viewMatrix = viewMatrix;
        terrainRenderer->worldMatrix = Matrix4::IDENTITY;
        terrainRenderer->projectionMatrix = projectionMatrix;
        terrainRenderer->Render();
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    gameWorld->Render(viewMatrix, projectionMatrix);
    
    
    
    
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
    
    
        
    
    
    line3DRenderer->SetColor(TColor::Red);
    
       
    
    
    line3DRenderer->Flush();
    flatRenderer->Flush();
    renderer->Flush();
    
    glDisable(GL_BLEND);
   

   
    
    
    glDisable(GL_DEPTH_TEST);
    mainController->Setup3DCamera();
    gameWorld->RenderMessages();
    
    
    flatRenderer->SetOrtho2D(0.0f, 1024.0f, 768.0f, 0.0f);
    
    
    // Render Information
    #if 0
    mainController->RenderText(200.0f, 50.0f, std::string("Time: ") + std::to_string(gameWorld->GetTotalPlayedTime()), 24, TColor::Yellow);
    #endif
    
#if 0
    // animals in zoo count, use position to determine
    int inAreaAnimalCount = gameWorld->GetInZooAnimalCount();
    mainController->RenderText(100.0f, 180.0f, std::string("In Zoo: ")+std::to_string(inAreaAnimalCount), 36, TColor::Yellow);
    int leavingZooAnimalCount = gameWorld->GetLeavingZooAnimalCount();
    mainController->RenderText(100.0f, 210.0f, std::string("Leaving Zoo: ")+std::to_string(leavingZooAnimalCount), 36, TColor::Yellow);    
#endif
    
    
    #if 0
    float w = 256.0f;
    float h = 256.0f;
    
    TFlatRenderer::TVertexData pt0(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    TFlatRenderer::TVertexData pt1(   w, 0.0f, 0.0f, 1.0f, 1.0f);
    TFlatRenderer::TVertexData pt2(   w,    h, 0.0f, 1.0f, 0.0f);
    TFlatRenderer::TVertexData pt3(0.0f,    h, 0.0f, 0.0f, 0.0f);
    
    flatRenderer->RenderTriangle(pt0, pt1, pt2);
    flatRenderer->RenderTriangle(pt0, pt2, pt3);
    flatRenderer->Flush();
    #endif
    
    
    mainController->Setup3DCamera();
    
    flatRenderer->Flush();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    {
        float w = 157.0f;
        float h = 63.0f;
        float x = 860.0f;
        float y = 0.0f;
        RenderSprite(activityLogoSprite, x, y, x + w, y + h);
    }
}

#endif

#if defined(__APPLE__)
#pragma mark - Touch Input
#endif


void AnimalsState::OnTouchDown(const std::vector<TTouchControl>& touches)
{
    if( gameWorld )
        gameWorld->OnTouchDown(touches);
}


void AnimalsState::OnScrollWheel(float delta)
{
   if( gameWorld )
       gameWorld->OnScrollWheel(delta);
    
    
    
    
    
    
    
    
    
    
}


void AnimalsState::OnTouchMoved(const std::vector<TTouchControl>& touches)
{
    if( gameWorld )
        gameWorld->OnTouchMoved(touches);
}


void AnimalsState::OnTouchUp(const std::vector<TTouchControl>& touches)
{
    if( gameWorld )
        gameWorld->OnTouchUp(touches);
}


void AnimalsState::OnPinched(float scale, float x, float y, float dx, float dy)
{
    if( gameWorld )
        gameWorld->OnPinched(scale, x, y, dx, dy);
}

void AnimalsState::OnResize(float width, float height)
{
    if( gameWorld )
        gameWorld->OnResize(width, height);
}


void AnimalsState::FixCameraPositionFromScreenResolution()
{
    #if 0
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


void AnimalsState::OnEnter()
{
    printf("%s\n",__FUNCTION__);
    if( ! animalStateImp )
        return;
    
#if VR_BUILD
    settings.nearPlane = GameStatics::GetInstance()->nearPlane;
    settings.farPlane = GameStatics::GetInstance()->farPlane;
#endif
    
    
    playerShouldMove = false;
    playerPos = Vector3(0.0f, 100.0f, 0.0f);
    totalPlayedTime = 0.0f;
    
    
    FixCameraPositionFromScreenResolution();

    
    if( animalStateImp )
       [animalStateImp onEnter];
    
    
}

void AnimalsState::OnLeave()
{
    printf("%s\n",__FUNCTION__);
}




