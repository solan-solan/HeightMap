/*
The class which allows to render multiple skinned objects statically batched to one model with own animation for each single patch

The model must have VERTEX_ATTRIB_TEX_COORD1 attribute for verticies, where uv.x is equal to the number of the certain patch

create_batch.py helps to create such a model, look its description

(Supports models with one mesh)
*/
#include "cocos2d.h"

namespace hm
{
    class HeightMap;
}

namespace model
{

    struct SkinBatch : public ax::Node
    {

    protected:

        int _patch_num = 0;
        std::string _mdl_path;

        float _skin_bone_number = 0.f;
        float _float_text_size = 0.f;
        int _fill_lines = 0.f;
        ax::Texture2D* _f_text = nullptr; // Float texture to pass [Matrix_Pallite, Model Matrix, Normal Matrix] to the shader

        // Shaders to process float texture
        const std::string _vert_shader = "res/shaders/model3d_skin.vert";
        const std::string _frag_shader = "res/shaders/model3d.frag";

        float* _tmp = nullptr;

    protected:

        SkinBatch(const std::string& mdl_path, int patch_num) : _mdl_path(mdl_path), _patch_num(patch_num)
        {}

        void getNormalMat3OfMat4(const cocos2d::Mat4& mat, float normalMat[]);

    public:

        ~SkinBatch();

        static SkinBatch* create(const std::string& model_path, int patch_number);

        virtual bool init() override;

        virtual void visit(cocos2d::Renderer* renderer, const cocos2d::Mat4& parentTransform, uint32_t parentFlags) override;

        // Update uniforms to float texture
        virtual void update(float time) override;

        void setScale(float scale, int num);
        void setPosition3D(const cocos2d::Vec3& pos, int num);
        void setRotationQuat(const cocos2d::Quaternion& q, int num);
        void setRotation3D(const cocos2d::Vec3& rot, int num);
        void runAction(cocos2d::Action* ac, int num);

        // Random populate circle with 'center' and 'radius', hm height will be taken if it was represented
        // No collision detection
        void randomPopulate(const cocos2d::Vec3& center, float radius, float scale_from, float scale_to, const hm::HeightMap* hm = nullptr);

        // Action with random delay
        void runActionAllLoop(const cocos2d::Vector<cocos2d::FiniteTimeAction*>& ac, float time_from, float time_to);

    };

}
