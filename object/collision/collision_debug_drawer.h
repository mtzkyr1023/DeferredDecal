#ifndef _COLLISION_DEBUG_DRAWER_H_
#define _COLLISION_DEBUG_DRAWER_H_


#include <vector>

#include "../../glm-master/glm/glm.hpp"
#include "../../glm-master/glm/gtc/matrix_transform.hpp"
#include "../../glm-master/glm/gtc/type_ptr.hpp"

#include "btBulletCollisionCommon.h"


class CollisionDebugDraw : public btIDebugDraw {
public:
	CollisionDebugDraw();
	virtual ~CollisionDebugDraw();

    // ���̕`��
    virtual void    drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
    // �Փ˓_�i�����t���j�̕`��
    virtual void    drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);
    // �x���A�G���[���̏o��
    virtual void    reportErrorWarning(const char* warningString);
    // 3D�����̕`��
    virtual void    draw3dText(const btVector3& location, const char* textString);
    // �f�o�b�O���[�h�̐ݒ�
    virtual void    setDebugMode(int debugMode);
    // �f�o�b�O���[�h�̎擾
    virtual int     getDebugMode() const;

    void clearLines();

public:
    struct LineInfo {
        glm::vec3 pos;
        glm::vec3 color;
        float padding[2];
    };


    const std::vector<LineInfo>& getLineBuffer() { return m_lineBuffer; }

private:
    std::vector<LineInfo> m_lineBuffer;

    int m_lineCount;

    int m_debugMode;

public:
    int getLineCount() { return m_lineCount; }
};

#endif