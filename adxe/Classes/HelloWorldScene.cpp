/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "HelloWorldScene.h"
#include "hm/ShadowCamera.h"
#include "pp/RTSprite.h"
#include "pp/FilterBase.h"
#include "pp/FilterDof.h"
#include "pp/RenderTexture3D.h"

USING_NS_CC;

#define DIRECTION_COLOR Vec3(0.4f, 0.4f, 0.4f)
#define AMBIENT_COLOR_FROM Vec3(1.f, 1.f, 1.f)
#define AMBIENT_COLOR_TO Vec3(1.f, 1.f, 1.f)
#define FOG_COLOR_FROM Vec3(204.f / 255.f * 0.88f, 206.f / 255.f * 0.88f, 184.f / 255.f * 0.88f)
#define FOG_COLOR_TO FOG_COLOR_FROM
#define SHADOW_CAM_DIST_WORLD 256.f
#define DEPTH_TEXT_SIZE_WORLD 2048.f
#define SHADOW_CAM_DIST_LOCAL 2.f
#define DEPTH_TEXT_SIZE_LOCAL 1024.f

std::vector<pp::RTSprite*> _filters;

HelloWorld::~HelloWorld()
{
    for (auto rt_spr : _filters)
        rt_spr->release();
}

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    if (closeItem == nullptr ||
        closeItem->getContentSize().width <= 0 ||
        closeItem->getContentSize().height <= 0)
    {
        problemLoading("'CloseNormal.png' and 'CloseSelected.png'");
    }
    else
    {
        float x = origin.x + visibleSize.width - closeItem->getContentSize().width/2;
        float y = origin.y + closeItem->getContentSize().height/2;
        closeItem->setPosition(Vec2(x,y));
    }

    // create menu, it's an autorelease object
    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    /////////////////////////////
    // 3. add your codes below...
    setCameraMask((unsigned short)CameraFlag::USER1 | (unsigned short)CameraFlag::USER4);

    _visit[int(CameraFlag::USER1) >> 2] = &HelloWorld::visit_cmn;
    _visit[int(CameraFlag::USER2) >> 2] = &HelloWorld::visit_shadow_local;
    _visit[int(CameraFlag::USER3) >> 2] = &HelloWorld::visit_shadow_world;
    _visit[int(CameraFlag::USER4) >> 2] = &HelloWorld::visit_pp;

	Director::getInstance()->setClearColor(Color4F(FOG_COLOR_FROM.x, FOG_COLOR_FROM.y, FOG_COLOR_FROM.z, 1.f));

    const auto& sz = Director::getInstance()->getWinSize();

    // Init default camera as ortho
    class CameraCocos : public Camera
    {
    public:

        void initOrtho(float zoomX, float zoomY, float nearPlane, float farPlane)
        {
            _zoom[0] = zoomX;
            _zoom[1] = zoomY;
            _nearPlane = nearPlane;
            _farPlane = farPlane;
            Mat4::createOrthographicOffCenter(0, _zoom[0], 0, _zoom[1], _nearPlane, _farPlane, &_projection);
            _viewProjectionDirty = true;
            _frustumDirty = true;
            _type = Type::ORTHOGRAPHIC;
        }
    };
    static_cast<CameraCocos*>(_defaultCamera)->initOrtho(sz.width, sz.height, -1024.f, 1024.f);
    _defaultCamera->setPosition3D(Vec3(0.f, 0.f, 0.f));
    _defaultCamera->setRotation3D(Vec3(0.f, 0.f, 0.f));
    _defaultCamera->setCameraFlag(CameraFlag::USER4);

    // Add 3d camera
    _cam = Camera::createPerspective(60.f, sz.width / sz.height, 0.1f, 800.f);
    addChild(_cam);
    _cam->setCameraFlag(CameraFlag::USER1);

    // Add lights
    _ambientLight = AmbientLight::create(Color3B(AMBIENT_COLOR_FROM.x * 255.f,
        AMBIENT_COLOR_FROM.y * 255.f,
        AMBIENT_COLOR_FROM.z * 255.f));
    _ambientLight->setEnabled(true);
    addChild(_ambientLight);
    _ambientLight->setCameraMask((unsigned short)CameraFlag::USER1);

    Vec3 dir_l = Vec3(-1.f, -0.5f, 0.f);
    _directionalLight = DirectionLight::create(dir_l, Color3B(DIRECTION_COLOR.x * 255.f,
        DIRECTION_COLOR.y * 255.f,
        DIRECTION_COLOR.z * 255.f));
    _directionalLight->setEnabled(true);
    addChild(_directionalLight);
    _directionalLight->setCameraMask((unsigned short)CameraFlag::USER1);

    // Add player
    _player = Sprite3D::create("res/model/girl.c3b");
    _player->setCameraMask((unsigned short)CameraFlag::USER1 | (unsigned short)CameraFlag::USER2);
    _player->setScale(0.0025f);
    addChild(_player);

    // Set control settings
    auto rect = _player->getBoundingBox();
    
    _player_settings._height = rect.size.height;
    _player_settings._width = rect.size.width;
    _player_settings._speed = _player_settings._width * 0.01f;

    _settings._min_dist = _player_settings._width * 0.5f;
    _settings._max_dist = _player_settings._width * 3.f;
    _settings._min_height = _player_settings._height * 0.2f;
    _settings._max_height = _player_settings._height * 1.5f;
    _settings._cur_dist = (_settings._max_dist + _settings._min_dist) * 0.5f;
    _settings._cur_height = (_settings._min_height + _settings._max_height) * 0.5f;
    _settings._gor_vel = _player->getScale() * 15.f;
    _settings._vert_vel = _player->getScale() * 0.1f;

    // Create world shadow camera
    _shdw_cam_world = hm::ShadowCamera::create(SHADOW_CAM_DIST_WORLD, SHADOW_CAM_DIST_WORLD, SHADOW_CAM_DIST_WORLD * 4.f, DEPTH_TEXT_SIZE_WORLD);
    _shdw_cam_world->setCameraFlag(CameraFlag::USER3);
    _shdw_cam_world->setDepth(-2);
    addChild(_shdw_cam_world);

    // Create local shadow camera
    _shdw_cam_local = hm::ShadowCamera::create(SHADOW_CAM_DIST_LOCAL, SHADOW_CAM_DIST_LOCAL, SHADOW_CAM_DIST_LOCAL * 4.f, DEPTH_TEXT_SIZE_LOCAL);
    _shdw_cam_local->setCameraFlag(CameraFlag::USER2);
    _shdw_cam_local->setDepth(-3);
    addChild(_shdw_cam_local);

    // Add particle sun
    _sun = BillBoard::create();
    _sun->setCameraMask((unsigned short)CameraFlag::USER1);
    _sun->setScale(0.002f);
    addChild(_sun);

    auto* prtcl = ParticleSun::create();
    prtcl->setTexture(Director::getInstance()->getTextureCache()->addImage("res/texture/fire.png"));
    prtcl->setCameraMask((unsigned short)CameraFlag::USER1);
    prtcl->setScale(200.f);
    prtcl->setPosition(Vec2(0.f, 0.f));
    _sun->addChild(prtcl);
    
    // Create heightMap
    // (Supported 1 shadow camera)
    _height_map = hm::HeightMap::create("res/hm/prop", { _shdw_cam_local, _shdw_cam_world });

    // Load first part of the heights
    _height_map->loadHeightsFromFile("res/hm/height/region_1.c3b", 
        { "res/hm/height/r1l1.png", "res/hm/height/r1l2.png" });

    // Load second part of the heights
    _height_map->loadHeightsFromFile("res/hm/height/region_2.c3b",
        { "res/hm/height/r2l1.png", "res/hm/height/r2l2.png" });

    addChild(_height_map, 1000);
    _height_map->setFog(100.f, 500.f);
    _height_map->setAtmosphereLight(dir_l, DIRECTION_COLOR, AMBIENT_COLOR_FROM, FOG_COLOR_FROM);
    _height_map->loadTextures();

    // Add house
    auto* house = Sprite3D::create("res/model/house.c3b");
    house->setCameraMask((unsigned short)CameraFlag::USER1 | (unsigned short)CameraFlag::USER2);

    house->setScale(house->getScale() *
        _height_map->getProperty()._scale.x *
        (1.f / _height_map->getProperty()._load_scale));

    house->setPosition3D(house->getPosition3D() * _height_map->getProperty()._scale.x *
        (1.f / _height_map->getProperty()._load_scale));

    Vec3 n;
    house->setPositionY(_height_map->getHeight(house->getPositionX(), house->getPositionZ(), &n));
    addChild(house);

    // Set player position base on house position
    _player->setPositionX(house->getPositionX());
    _player->setPositionZ(house->getPositionZ() + 4.f);
    _player->setPositionY(_height_map->getHeight(_player->getPositionX(), _player->getPositionZ(), &n));
    setCameraBehind(true);

    // Set skybox
    TextureCube* textureCube = TextureCube::create("res/skybox/left.jpg", "res/skybox/right.jpg",
        "res/skybox/top.jpg", "res/skybox/bottom.jpg",
        "res/skybox/front.jpg", "res/skybox/back.jpg");

    Texture2D::TexParams tRepeatParams;
    tRepeatParams.magFilter = backend::SamplerFilter::LINEAR;
    tRepeatParams.minFilter = backend::SamplerFilter::LINEAR;
    tRepeatParams.sAddressMode = backend::SamplerAddressMode::CLAMP_TO_EDGE;
    tRepeatParams.tAddressMode = backend::SamplerAddressMode::CLAMP_TO_EDGE;
    textureCube->setTexParameters(tRepeatParams);

    Skybox* sky_box = Skybox::create();
    sky_box->setCameraMask((unsigned short)CameraFlag::USER1);
    sky_box->setTexture(textureCube);
    addChild(sky_box, 10000);

    // Add ui
    addUi();

    // Init control events
    auto listener = EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = CC_CALLBACK_2(HelloWorld::onTouchesBegan, this);
    listener->onTouchesMoved = CC_CALLBACK_2(HelloWorld::onTouchesMoved, this);
    listener->onTouchesEnded = CC_CALLBACK_2(HelloWorld::onTouchesEnd, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    // Init pp filters
    _filters.resize(2);

    // Base filter (renders to screen as is)
//    auto flt = new pp::FilterBase(sz, true);
//    flt->init();
//    _filters.at(0) = pp::RTSprite::create(flt, sz);
//    _filters.at(0)->setCameraMask((unsigned short)CameraFlag::USER4);
//    _filters.at(0)->retain();

    // Dof filter
    auto flt_1 = new pp::FilterDof(sz, Vec4(0.f, 2.f / sz.height, _cam->getNearPlane(), _cam->getFarPlane()), 8, true);
    flt_1->setDepthCoeff(5.f);
    _filters.at(0) = pp::RTSprite::create(flt_1, sz);
    _filters.at(0)->setCameraMask((unsigned short)CameraFlag::USER4);
    _filters.at(0)->retain();

    auto flt_2 = new pp::FilterDof(sz, Vec4(2.f / sz.width, 0.f, _cam->getNearPlane(), _cam->getFarPlane()), 8, false);
    flt_2->setDepthTexture(flt_1->getRenderTexture()->getDepthText());
    flt_2->setDepthCoeff(5.f);
    _filters.at(1) = pp::RTSprite::create(flt_2, sz);
    _filters.at(1)->setCameraMask((unsigned short)CameraFlag::USER4);
    _filters.at(1)->retain();

    // Start update to pass 'time passed' to the HeightMap (Its needed to animate the grass)
    scheduleUpdate();

    return true;
}


void HelloWorld::menuCloseCallback(Ref* pSender)
{
    //Close the cocos2d-x game scene and quit the application
    Director::getInstance()->end();

    /*To navigate back to native iOS screen(if present) without quitting the application  ,do not use Director::getInstance()->end() as given above,instead trigger a custom event created in RootViewController.mm as below*/

    //EventCustom customEndEvent("game_scene_close_event");
    //_eventDispatcher->dispatchEvent(&customEndEvent);
}

void HelloWorld::onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
{
    auto touch = touches[0];
    auto location = touch->getLocationInView();
    auto size = Director::getInstance()->getVisibleSize();
    if (location.x < size.width * 0.5f)
    {
        // Action to move in the world
        struct ActionMove : public ActionInterval
        {
        private:

            HelloWorld* _scene = nullptr;

        public:

            ActionMove(HelloWorld* sc): _scene(sc)
            {
            }

            virtual void update(float time) override
            {
                if (time != 0.f && time != 1.f)
                {
                    Vec3 f;
                    _scene->getNodeToWorldTransform().getForwardVector(&f);
                    Vec3 p = _target->getPosition3D() + -_scene->_player_settings._forward * _scene->_player_settings._speed;
                    Vec3 n;
                    p.y = _scene->_height_map->getHeight(p.x, p.z, &n);
                    _scene->_player->setPosition3D(p);
                    _scene->setCameraBehind();
                }
            }

            friend HelloWorld;
        };

        auto* ac = new ActionMove(this);
        ac->autorelease();
        ac->initWithDuration(1.f);

        _player->runAction(RepeatForever::create(ac));

        // Action with the animation
        auto animation = Animation3D::create("res/model/girl.c3b", "Take 001");
        if (animation)
        {
            auto animate = Animate3D::create(animation);
            _player->runAction(RepeatForever::create(animate));
        }
    }
}

void HelloWorld::onTouchesMoved(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
{
    auto touch = touches[0];
    Vec2 delta =  touch->getDelta();
    float angle = CC_DEGREES_TO_RADIANS(-delta.x) * _settings._gor_vel;

    // Rotate player
    auto from = _player->getRotationQuat();
    Quaternion rot, to;
    Quaternion::createFromAxisAngle(Vec3(0.f, 1.f, 0.0), angle, &rot);
    to = from * rot;
    _player->setRotationQuat(to);

    // Adjust camera height
    _settings._cur_dist += delta.y * _settings._vert_vel;
    _settings._cur_height += delta.y * _settings._vert_vel;
    if (_settings._cur_dist > _settings._max_dist)
        _settings._cur_dist = _settings._max_dist;
    if (_settings._cur_dist < _settings._min_dist)
        _settings._cur_dist = _settings._min_dist;
    if (_settings._cur_height > _settings._max_height)
        _settings._cur_height = _settings._max_height;
    if (_settings._cur_height < _settings._min_height)
        _settings._cur_height = _settings._min_height;

    // Set camera behind
    setCameraBehind();
}

void HelloWorld::onTouchesEnd(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event)
{
    _player->stopAllActions();
}

void HelloWorld::setCameraBehind(bool force_world_shadow_cam)
{
    Vec3 back, right;
    _player->getNodeToWorldTransform().getRightVector(&right);
    _player->getNodeToWorldTransform().getBackVector(&back);

    _player_settings._forward = -back + right;
    _player_settings._forward.normalize();

    Vec3 pos_player = _player->getPosition3D();
    Vec3 pos_cam = pos_player + _player_settings._forward * _settings._cur_dist;

    Vec3 n;
    float h = _height_map->getHeight(pos_cam.x, pos_cam.z, &n);
    if (h < pos_player.y)
    {
        pos_cam.y = pos_player.y + _settings._cur_height;
    }
    else
    {
        pos_cam.y = h + _settings._cur_height;
    }

    _cam->setPosition3D(pos_cam);
    _cam->lookAt(Vec3(pos_player.x, pos_player.y + _player_settings._height * 0.7f, pos_player.z));

    updateSdwCamPos(pos_player, _directionalLight->getDirection(), force_world_shadow_cam);
}

void HelloWorld::update(float time)
{
    _time_passed += time;
    _height_map->setTimePassed(_time_passed);
    _height_map->updatePositionView(_player->getPosition3D());
    _height_map->updateHeightMap();
}

void HelloWorld::addUi()
{
    auto sz = Director::getInstance()->getVisibleSize();
    float sc_fct = 1.f / Director::getInstance()->getContentScaleFactor();

    //--- Grass
    auto* txt = ui::Text::create("grass", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f + 200.f * sc_fct));
    txt->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(txt);

    auto* cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f + 200.f * sc_fct));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::enableGrass, this));
    cb->setScale(3.f * sc_fct);
    cb->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(cb);

    //--- Grid
    txt = ui::Text::create("grid", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f));
    txt->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(txt);

    cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::showGrid, this));
    cb->setScale(3.f * sc_fct);
    cb->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(cb);

    //--- Normal
    txt = ui::Text::create("normal", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f - 200.f * sc_fct));
    txt->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(txt);

    cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f - 200.f * sc_fct));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::showNormal, this));
    cb->setScale(3.f * sc_fct);
    cb->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(cb);

    //--- Speed
    txt = ui::Text::create("speed", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width / 2.f - 100.f * sc_fct, 50.f * sc_fct));
    txt->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(txt);

    ui::Slider* sl = ui::Slider::create();
    sl->setAnchorPoint(Vec2(0.f, 0.5f));
    sl->loadBarTexture("res/ui/sliderTrack.png");
    sl->loadSlidBallTextures("re/ui/sliderThumb.png", "res/ui/sliderThumb.png", "");
    sl->loadProgressBarTexture("res/ui/sliderProgress.png");
    sl->setMaxPercent(10);
    sl->setPosition(Vec2(sz.width / 2.f, 50.f * sc_fct));
    sl->addEventListener(CC_CALLBACK_2(HelloWorld::changeSpeed, this));
    sl->setScale(3.f * sc_fct);
    sl->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(sl);

    //--- Sun
    txt = ui::Text::create("sun", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width / 2.f - 100.f * sc_fct, sz.height - 50.f * sc_fct));
    txt->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(txt);

    sl = ui::Slider::create();
    sl->setAnchorPoint(Vec2(0.f, 0.5f));
    sl->loadBarTexture("res/ui/sliderTrack.png");
    sl->loadSlidBallTextures("re/ui/sliderThumb.png", "res/ui/sliderThumb.png", "");
    sl->loadProgressBarTexture("res/ui/sliderProgress.png");
    sl->setMaxPercent(100);
    sl->setPosition(Vec2(sz.width / 2.f, sz.height - 50.f * sc_fct));
    sl->addEventListener(CC_CALLBACK_2(HelloWorld::sunDir, this));
    sl->setScale(3.f * sc_fct);
    sl->setCameraMask((unsigned short)CameraFlag::USER4);
    addChild(sl);
}

void HelloWorld::enableGrass(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type)
{
    switch (type)
    {
    case ui::CheckBox::EventType::SELECTED:
        _height_map->enableGrass();
        _height_map->launchGrassUpdThread();
        _height_map->forceUpdateHeightMap(_height_map->getProperty()._lod_count);
        break;

    case ui::CheckBox::EventType::UNSELECTED:
        _height_map->stopGrassUpdThread();
        _height_map->disableGrass();
        break;

    default:
        break;
    }
}

void HelloWorld::showGrid(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type)
{
    switch (type)
    {
    case ui::CheckBox::EventType::SELECTED:
        _height_map->showGrid(true);
        _height_map->forceUpdateHeightMap(_height_map->getProperty()._lod_count);
        break;

    case ui::CheckBox::EventType::UNSELECTED:
        _height_map->showGrid(false);
        break;

    default:
        break;
    }
}

void HelloWorld::showNormal(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type)
{
    switch (type)
    {
    case ui::CheckBox::EventType::SELECTED:
        _height_map->showNormals(0.4f);
        _height_map->forceUpdateHeightMap(_height_map->getProperty()._lod_count);
        break;

    case ui::CheckBox::EventType::UNSELECTED:
        _height_map->showNormals(0.f);
        break;

    default:
        break;
    }
}

void HelloWorld::changeSpeed(Ref* pSender, ui::Slider::EventType type)
{
    if (type == ui::Slider::EventType::ON_PERCENTAGE_CHANGED)
    {
        ui::Slider* slider = dynamic_cast<ui::Slider*>(pSender);
        int percent = slider->getPercent() + 1;
        _player_settings._speed = _player_settings._width * 0.01f * (percent << 1);
    }
}

void HelloWorld::sunDir(Ref* pSender, cocos2d::ui::Slider::EventType type)
{
    if (type == ui::Slider::EventType::ON_PERCENTAGE_CHANGED)
    {
        ui::Slider* slider = dynamic_cast<ui::Slider*>(pSender);
        float alpha = float(slider->getPercent()) / float(slider->getMaxPercent());
        float dir_x = MathUtil::lerp(-1.f, 1.f, alpha);
        Vec3 dir_l = Vec3(dir_x, -0.5f, 0.f);
        Vec3 amb_color = AMBIENT_COLOR_FROM;
        amb_color = amb_color.lerp(AMBIENT_COLOR_TO, tweenfunc::cubicEaseIn(alpha));
        Vec3 fog_color = FOG_COLOR_FROM;
        fog_color = fog_color.lerp(FOG_COLOR_TO, tweenfunc::cubicEaseIn(alpha));
        _height_map->setAtmosphereLight(dir_l, DIRECTION_COLOR, amb_color, fog_color);
    
        _directionalLight->setDirection(dir_l);
        _ambientLight->setColor(Color3B(amb_color.x * 255, amb_color.y * 255, amb_color.z * 255));
    
        setCameraBehind(true);
    }
}

void HelloWorld::updateSdwCamPos(const cocos2d::Vec3& pos, const cocos2d::Vec3& dir_l, bool force)
{
    cocos2d::Vec3 p_look = Vec3(0.f, ONE_HEIGHT_DEF_H, pos.z);
    cocos2d::Vec3 p = p_look - _directionalLight->getDirection() * SHADOW_CAM_DIST_WORLD;
    _sun->setPosition3D(p);

    if (_shdw_cam_world)
    {
        float z_dist = std::abs(_shdw_cam_world->getPosition3D().z - pos.z);

        if ((z_dist > SHADOW_CAM_DIST_WORLD / 2.f) || force)
        {
            Vec3 frw;
            _cam->getNodeToWorldTransform().getForwardVector(&frw);
            frw.y = 0.f;

            _shdw_cam_world->setPosition3D(p);
            _shdw_cam_world->lookAt(p_look + frw * 0.0001f);
        }
    }

    if (_shdw_cam_local)
    {
        Vec3 frw;
        _cam->getNodeToWorldTransform().getForwardVector(&frw);
        frw.y = 0.f;

        cocos2d::Vec3 p = pos - dir_l * SHADOW_CAM_DIST_LOCAL;

        Vec3 n;
        float h = _height_map->getHeight(p.x, p.z, &n);
        if (h > p.y)
            p.y = h + 0.1f;

        // Set local camera
        _shdw_cam_local->setPosition3D(p);
        _shdw_cam_local->lookAt(pos + frw * 0.0001f);
    }
}

void HelloWorld::visit(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    (this->*_visit[int(Camera::getVisitingCamera()->getCameraFlag()) >> 2])(renderer, parentTransform, parentFlags);
}

void HelloWorld::visit_pp(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    // IMPORTANT:
    // To ease the migration to v3.0, we still support the Mat4 stack,
    // but it is deprecated and your code should not rely on it
    _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    if (_filters.size())
    {
        for (int i = 1; i < _filters.size(); ++i)
        {
            _filters[i]->begin();
            _filters[i - 1]->visit(renderer, _modelViewTransform, flags);
            _filters[i]->end();
        }

        // Draw last filtered sprite in stack on the screen
        _filters[_filters.size() - 1]->visit(renderer, _modelViewTransform, flags);
    }

    // Draw ui (should be optimized)
    for (auto it = _children.cbegin(), itCend = _children.cend(); it != itCend; ++it)
        (*it)->visit(renderer, _modelViewTransform, flags);

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    // FIX ME: Why need to set _orderOfArrival to 0??
    // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
    // reset for next frame
    // _orderOfArrival = 0;
}

void HelloWorld::visit_cmn(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
{
    // quick return if not visible. children won't be drawn.
    if (!_visible)
    {
        return;
    }

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    // IMPORTANT:
    // To ease the migration to v3.0, we still support the Mat4 stack,
    // but it is deprecated and your code should not rely on it
    _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    // Draw 3d scene to texture
    _filters[0]->begin();

    int i = 0;
    if (!_children.empty())
    {
        sortAllChildren();
        // draw children zOrder < 0
        for (auto size = _children.size(); i < size; ++i)
        {
            auto node = _children.at(i);

            if (node && node->getLocalZOrder() < 0)
                node->visit(renderer, _modelViewTransform, flags);
            else
                break;
        }

        for (auto it = _children.cbegin() + i, itCend = _children.cend(); it != itCend; ++it)
            (*it)->visit(renderer, _modelViewTransform, flags);
    }

    _filters[0]->end();

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    // FIX ME: Why need to set _orderOfArrival to 0??
    // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
    // reset for next frame
    // _orderOfArrival = 0;
}

void HelloWorld::visit_shadow_local(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
{
    // quick return if not visible. children won't be drawn.
    if (!_visible)
        return;

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    // IMPORTANT:
    // To ease the migration to v3.0, we still support the Mat4 stack,
    // but it is deprecated and your code should not rely on it
    _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    const auto* rt_cam = static_cast<const hm::ShadowCamera*>(Camera::getVisitingCamera());
    
    rt_cam->begin_render();

    int i = 0;
    if (!_children.empty())
    {
        sortAllChildren();
        // draw children zOrder < 0
        for (auto size = _children.size(); i < size; ++i)
        {
            auto node = _children.at(i);

            if (node && node->getLocalZOrder() < 0)
                node->visit(renderer, _modelViewTransform, flags);
            else
                break;
        }

        for (auto it = _children.cbegin() + i, itCend = _children.cend(); it != itCend; ++it)
            (*it)->visit(renderer, _modelViewTransform, flags);
    }

    rt_cam->end_render();

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    // FIX ME: Why need to set _orderOfArrival to 0??
    // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
    // reset for next frame
    // _orderOfArrival = 0;
}

void HelloWorld::visit_shadow_world(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
{
    // quick return if not visible. children won't be drawn.
    if (!_visible)
        return;

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    // IMPORTANT:
    // To ease the migration to v3.0, we still support the Mat4 stack,
    // but it is deprecated and your code should not rely on it
    _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    const auto* rt_cam = static_cast<const hm::ShadowCamera*>(Camera::getVisitingCamera());

    rt_cam->begin_render();

    _height_map->draw_shadow(renderer, _modelViewTransform, flags);

    rt_cam->end_render();

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    // FIX ME: Why need to set _orderOfArrival to 0??
    // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
    // reset for next frame
    // _orderOfArrival = 0;
}
