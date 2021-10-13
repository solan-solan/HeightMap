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

USING_NS_CC;

#define DIRECTION_COLOR Vec3(0.4f, 0.4f, 0.4f)
#define AMBIENT_COLOR_FROM Vec3(1.f, 1.f, 1.f)
#define AMBIENT_COLOR_TO Vec3(1.f, 1.f, 1.f)
#define FOG_COLOR_FROM Vec3(204.f / 255.f * 0.88f, 206.f / 255.f * 0.88f, 184.f / 255.f * 0.88f)
#define FOG_COLOR_TO FOG_COLOR_FROM
#define SHADOW_CAM_DIST 2.f
#define DEPTH_TEXT_SIZE 1024.f

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

    _visit[int(CameraFlag::USER1) >> 2] = &HelloWorld::visit_cmn;
    _visit[int(CameraFlag::USER2) >> 2] = &HelloWorld::visit_shadow;

	Director::getInstance()->setClearColor(Color4F(FOG_COLOR_FROM.x, FOG_COLOR_FROM.y, FOG_COLOR_FROM.z, 1.f));

    // Add 3d camera
    const auto& sz = Director::getInstance()->getWinSize();
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

    Vec3 dir_l = Vec3(-1.f, -1.f, 0.f);
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

    // Create shadow camera
    _shdw_cam = hm::ShadowCamera::create(SHADOW_CAM_DIST, SHADOW_CAM_DIST, SHADOW_CAM_DIST * 4.f, DEPTH_TEXT_SIZE);
    _shdw_cam->setCameraFlag(CameraFlag::USER2);
    _shdw_cam->setDepth(-3);
    addChild(_shdw_cam);

    // Add particle sun
    _sun = BillBoard::create();
    _sun->setCameraMask((unsigned short)CameraFlag::USER1);
    _sun->setScale(0.002f);
    addChild(_sun);

    auto* prtcl = ParticleSun::create();
    prtcl->setTexture(Director::getInstance()->getTextureCache()->addImage("res/texture/fire.png"));
    prtcl->setCameraMask((unsigned short)CameraFlag::USER1);
    prtcl->setScale(4.f);
    prtcl->setPosition(Vec2(0.f, 0.f));
    _sun->addChild(prtcl);
    
    // Create heightMap
    // (Supported 1 shadow camera)
    _height_map = hm::HeightMap::create("res/hm/prop", { _shdw_cam });

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
    setCameraBehind();

    // Launch HeightMap updating
    _height_map->forceUpdateHeightMap(Vec3(_player->getPositionX(), 0.f, _player->getPositionZ()));
    _height_map->launchUpdThread();

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

void HelloWorld::setCameraBehind()
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

    if (_shdw_cam)
        updateSdwCamPos(pos_player, _directionalLight->getDirection());
}

void HelloWorld::update(float time)
{
    _time_passed += time;
    _height_map->setTimePassed(_time_passed);
    _height_map->updatePositionView(_player->getPosition3D());
}

void HelloWorld::addUi()
{
    auto sz = Director::getInstance()->getVisibleSize();
    float sc_fct = 1.f / Director::getInstance()->getContentScaleFactor();

    //--- Grass
    auto* txt = ui::Text::create("grass", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f + 200.f * sc_fct));
    addChild(txt);

    auto* cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f + 200.f * sc_fct));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::enableGrass, this));
    cb->setScale(3.f * sc_fct);
    addChild(cb);

    //--- Grid
    txt = ui::Text::create("grid", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f));
    addChild(txt);

    cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::showGrid, this));
    cb->setScale(3.f * sc_fct);
    addChild(cb);

    //--- Normal
    txt = ui::Text::create("normal", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width - 200.f * sc_fct, sz.height / 2.f - 200.f * sc_fct));
    addChild(txt);

    cb = ui::CheckBox::create("res/ui/check_box_normal.png",
        "res/ui/check_box_normal_press.png",
        "res/ui/check_box_active.png",
        "res/ui/check_box_normal_disable.png",
        "res/ui/check_box_active_disable.png");
    cb->setPosition(Vec2(sz.width - 100.f * sc_fct, sz.height / 2.f - 200.f * sc_fct));
    cb->addEventListener(CC_CALLBACK_2(HelloWorld::showNormal, this));
    cb->setScale(3.f * sc_fct);
    addChild(cb);

    //--- Speed
    txt = ui::Text::create("speed", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width / 2.f - 100.f * sc_fct, 50.f * sc_fct));
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
    addChild(sl);

    //--- Sun
    txt = ui::Text::create("sun", "arial", 32 * sc_fct);
    txt->setPosition(Vec2(sz.width / 2.f - 100.f * sc_fct, sz.height - 50.f * sc_fct));
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
    addChild(sl);
}

void HelloWorld::enableGrass(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type)
{
    switch (type)
    {
    case ui::CheckBox::EventType::SELECTED:
        _height_map->enableGrass();
        _height_map->setGrassText("res/hm/grass/test_grass.png");
        _height_map->forceUpdateHeightMap(_height_map->getProperty()._lod_count);
        break;

    case ui::CheckBox::EventType::UNSELECTED:
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
        Vec3 dir_l = Vec3(dir_x, -1.f, 0.f);
        Vec3 amb_color = AMBIENT_COLOR_FROM;
        amb_color = amb_color.lerp(AMBIENT_COLOR_TO, tweenfunc::cubicEaseIn(alpha));
        Vec3 fog_color = FOG_COLOR_FROM;
        fog_color = fog_color.lerp(FOG_COLOR_TO, tweenfunc::cubicEaseIn(alpha));
        _height_map->setAtmosphereLight(dir_l, DIRECTION_COLOR, amb_color, fog_color);
    
        _directionalLight->setDirection(dir_l);
        _ambientLight->setColor(Color3B(amb_color.x * 255, amb_color.y * 255, amb_color.z * 255));
    
        setCameraBehind();
    }
}

void HelloWorld::updateSdwCamPos(const cocos2d::Vec3& pos, const cocos2d::Vec3& dir_l)
{
    cocos2d::Vec3 p = pos - dir_l * SHADOW_CAM_DIST;

    Vec3 n;
    float h = _height_map->getHeight(p.x, p.z, &n);
    if (h > p.y)
        p.y = h + 0.1f;

    Vec3 frw;
    _cam->getNodeToWorldTransform().getForwardVector(&frw);
    frw.y = 0.f;

    _sun->setPosition3D(p);
    _shdw_cam->setPosition3D(p);
    _shdw_cam->lookAt(pos + frw * 0.0001f);
}

void HelloWorld::visit(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    (this->*_visit[int(Camera::getVisitingCamera()->getCameraFlag()) >> 2])(renderer, parentTransform, parentFlags);
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

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);

    // FIX ME: Why need to set _orderOfArrival to 0??
    // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
    // reset for next frame
    // _orderOfArrival = 0;
}

void HelloWorld::visit_shadow(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags)
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
