#include "SkinBatch.h"
#include "renderer/backend/Device.h"
#include "hm/HeightMap.h"

using namespace cocos2d;
using namespace model;

SkinBatch::~SkinBatch()
{
    assert(_f_text->getReferenceCount() == 1);
    CC_SAFE_RELEASE_NULL(_f_text);

    AX_SAFE_DELETE_ARRAY(_tmp);
}

SkinBatch* SkinBatch::create(const std::string& model_path, int patch_number)
{
    SkinBatch* ret = new SkinBatch(model_path, patch_number);
    if (ret->init())
    {
        ret->autorelease();
    }
    else
    {
        AX_SAFE_DELETE(ret);
    }
    return ret;
}

bool SkinBatch::init()
{
    Node::init();

    // Create first model
    Sprite3D* first = Sprite3D::create(_mdl_path);
    
    auto* skin = first->getMesh()->getSkin();

    if (!skin)
    {
        first->release();
        return false; // The model does not have skin
    }

    _skin_bone_number = skin->getBoneCount();

    // Number of pixels for one patch
    int num_pix_for_patch = _skin_bone_number * 3 + 4 + 3; // [bone count * 3 matrix pallite rows on one bone + 4 matrix model + 3 normal matrix]
    // Whole pixels for all patches
    int need_pixels = num_pix_for_patch * _patch_num;

    // Set float texture size
    std::array<int, 8> sizes = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
    for (int i = 0; i < sizes.size(); ++i)
        if (sizes[i] * sizes[i] > need_pixels)
        {
            _float_text_size = sizes[i];
            break;
        }

    if (!_float_text_size)
    {
        first->release();
        return false;
    }

    // Intermediate array for data updating
    _tmp = new float[(_float_text_size * _float_text_size) * 4];

    // Create float texture
    _f_text = new ax::Texture2D();
    _f_text->setAliasTexParameters(); // NEAREST is needed

    // Create shader
    std::string def_model_sh = "#define MAX_DIRECTIONAL_LIGHT_NUM 1\n#define MAX_POINT_LIGHT_NUM 0\n#define MAX_SPOT_LIGHT_NUM 0\n";
    std::string vertexSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename(_vert_shader));
    std::string fragmentSource = FileUtils::getInstance()->getStringFromFile(FileUtils::getInstance()->fullPathForFilename(_frag_shader));
    auto* program = ax::backend::Device::getInstance()->newProgram(def_model_sh + vertexSource, def_model_sh + fragmentSource);
    program->autorelease();

    auto* prState = new ax::backend::ProgramState(program);
    
    auto loc_f_text = prState->getUniformLocation("u_tex_f");
    prState->setTexture(loc_f_text, 1, _f_text->getBackendTexture());

    auto loc_ft_size = prState->getUniformLocation("u_ft_size");
    prState->setUniform(loc_ft_size, &_float_text_size, sizeof(_float_text_size));
    
    auto loc_bone_num = prState->getUniformLocation("u_mesh_bone_num");
    prState->setUniform(loc_bone_num, &_skin_bone_number, sizeof(_skin_bone_number));

    auto* mat = Material::createWithProgramState(prState);
    mat->getStateBlock().setBlend(false);
    mat->getStateBlock().setBlendFunc(ax::BlendFunc::DISABLE);
    
    // Set corresponding material for the first created object, since it will be rendered
    first->setMaterial(mat);
    
    // Add first object
    addChild(first);

    // Create other objects which will not be rendered
    // Its creating does not cause to allocating of additional GL buffers, since they were bring from cache
    // They are needed to play animation and keep Matrix Model
    for (int i = 0; i < _patch_num - 1; ++i)
    {
        auto* other = Sprite3D::create(_mdl_path);
        addChild(other);
    }

    return true;
}

void SkinBatch::visit(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags)
{
    // quick return if not visible. children won't be drawn.
    if (!_visible)
    {
        return;
    }

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

    bool visibleByCamera = isVisitableByVisitingCamera();

    int i = 0;

    if (!_children.empty())
    {
        auto it = _children.cbegin();
        (*it++)->visit(renderer, _modelViewTransform, flags);

        while (it != _children.cend())
        {
            if (static_cast<Sprite3D*>(*it)->getSkeleton())
                static_cast<Sprite3D*>(*it)->getSkeleton()->updateBoneMatrix();
            it++;
        }
    }

    _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void SkinBatch::update(float time)
{
    int j = 0;
    for (auto* o : _children)
    {
        auto* m = static_cast<Sprite3D*>(o)->getMesh();
        
        // Add skin data
        ax::Vec4* mp = m->getSkin()->getMatrixPalette();
        auto mp_size = m->getSkin()->getMatrixPaletteSizeInBytes();

        for (int i = 0; i < m->getSkin()->getMatrixPaletteSize(); ++i)
        {
            _tmp[j] = mp[i].x;
            _tmp[j + 1] = mp[i].y;
            _tmp[j + 2] = mp[i].z;
            _tmp[j + 3] = mp[i].w;

            j += 4;
        }

        // Add model transform data
        Mat4 tr = o->getNodeToWorldTransform();
        for (int i = 0; i < sizeof(tr.m) / sizeof(float); ++i)
        {
            _tmp[j] = tr.m[i];
            j++;
        }

        // Add normal transform data
        auto tr_nrm = utils::getNormalMat3OfMat4(tr);
        for (int i = 0; i < tr_nrm.size(); i += 3)
        {
            _tmp[j] = tr_nrm[i];
            _tmp[j + 1] = tr_nrm[i + 1];
            _tmp[j + 2] = tr_nrm[i + 2];
            _tmp[j + 3] = 0.f;
            j += 4;
        }
    }

    _f_text->updateWithData(_tmp, (_float_text_size * _float_text_size) * 4,
        ax::backend::PixelFormat::RGBA32F,
        ax::backend::PixelFormat::RGBA32F,
        _float_text_size, _float_text_size,
        false, 0);
}

void SkinBatch::setScale(float scale, int num)
{
    if (num < _children.size())
        _children.at(num)->setScale(scale);
}

void SkinBatch::setPosition3D(const cocos2d::Vec3& pos, int num)
{
    if (num < _children.size())
        _children.at(num)->setPosition3D(pos);
}

void SkinBatch::setRotationQuat(const cocos2d::Quaternion& q, int num)
{
    if (num < _children.size())
        _children.at(num)->setRotationQuat(q);
}

void SkinBatch::setRotation3D(const cocos2d::Vec3& rot, int num)
{
    if (num < _children.size())
        _children.at(num)->setRotation3D(rot);
}

void SkinBatch::runAction(cocos2d::Action* ac, int num)
{
    if (num < _children.size())
        _children.at(num)->runAction(ac);
}

void SkinBatch::randomPopulate(const cocos2d::Vec3& center, float radius, float scale_from, float scale_to, const hm::HeightMap* hm)
{
    for (auto* ch : getChildren())
    {
        ch->setRotationQuat(Quaternion::identity());

        // Set Position
        Vec3 pos;
        pos.x = center.x + cocos2d::random(-radius, radius);
        pos.z = center.z + cocos2d::random(-radius, radius);
        pos.y = 0.f;

        if (hm)
        {
            Vec3 n;
            pos.y = hm->getHeight(pos.x, pos.z, &n);
        }

        ch->setPosition3D(pos);

        // Set Rotation
        float angle_y = cocos2d::random(-180.f, 180.f);
        ch->setRotation3D(Vec3(0.f, angle_y, 0.f));

        // Set scale
        float scale = cocos2d::random(scale_from, scale_to);
        ch->setScale(scale);
    }
}

void SkinBatch::runActionAllLoop(const cocos2d::Vector<cocos2d::FiniteTimeAction*>& ac, float time_from, float time_to)
{
    for (auto* ch : getChildren())
    {
        Vector<cocos2d::FiniteTimeAction*> ac_l;
        float delay = cocos2d::random(time_from, time_to);
        ac_l.pushBack(DelayTime::create(delay));
        for (auto* a : ac)
            ac_l.pushBack(a->clone());
        ch->runAction(RepeatForever::create(Sequence::create(ac_l)));
    }
}
