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

#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#include "hm/HeightMap.h"
#include "ui/CocosGUI.h"

namespace hm
{
    class ShadowCamera;
};

class HelloWorld : public cocos2d::Scene
{

private:

    cocos2d::Sprite3D* _player = nullptr;
    cocos2d::Camera* _cam = nullptr;
    hm::HeightMap* _height_map = nullptr;
    float _time_passed = 0.f;
    cocos2d::AmbientLight* _ambientLight = nullptr;
    cocos2d::DirectionLight* _directionalLight = nullptr;
    hm::ShadowCamera* _shdw_cam_world = nullptr;
    hm::ShadowCamera* _shdw_cam_local = nullptr;
    cocos2d::BillBoard* _sun = nullptr;

    // Vector of visit methods coresponding to the Camera flag
    typedef void(HelloWorld::* visit_ptr)(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags);
    std::array<visit_ptr, 3> _visit;

    struct CAMERA_SETTINGS
    {
        float _min_dist = 0.f;
        float _max_dist = 0.f;
        float _min_height = 0.f;
        float _max_height = 0.f;
        float _cur_dist = 0.f;
        float _cur_height = 0.f;
        float _gor_vel = 0.f;
        float _vert_vel = 0.f;
    } _settings;

    struct PLAYER_SETTINGS
    {
        float _height = 0.f;
        float _width = 0.f;
        float _speed = 0.f;
        cocos2d::Vec3 _forward;
    } _player_settings;

private:

    void setCameraBehind(bool force_world_shadow_cam = false);
    void updateSdwCamPos(const cocos2d::Vec3& pos, const cocos2d::Vec3& dir, bool force = false);
    void addUi();

    void enableGrass(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type);
    void showGrid(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type);
    void showNormal(cocos2d::Ref* sender, cocos2d::ui::CheckBox::EventType type);
    void changeSpeed(Ref* pSender, cocos2d::ui::Slider::EventType type);
    void sunDir(Ref* pSender, cocos2d::ui::Slider::EventType type);

public:
    static cocos2d::Scene* createScene();

    virtual bool init();
    virtual void visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags);
    void visit_cmn(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags);
    void visit_shadow_local(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags);
    void visit_shadow_world(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags);

    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);
    
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);

    void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
    void onTouchesMoved(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
    void onTouchesEnd(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);

    virtual void update(float time) override;
};

#endif // __HELLOWORLD_SCENE_H__
