#include "collision_debug_drawer.h"


CollisionDebugDraw::CollisionDebugDraw() :
	m_debugMode(0),
	m_lineCount(0)
{
	m_lineBuffer.resize(2048);
}

CollisionDebugDraw::~CollisionDebugDraw() {

}


void CollisionDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	if (m_lineBuffer.size() > m_lineCount) {
		m_lineBuffer[m_lineCount].pos.x = from.x();
		m_lineBuffer[m_lineCount].pos.y = from.y();
		m_lineBuffer[m_lineCount].pos.z = from.z();

		m_lineBuffer[m_lineCount + 1].pos.x = to.x();
		m_lineBuffer[m_lineCount + 1].pos.y = to.y();
		m_lineBuffer[m_lineCount + 1].pos.z = to.z();

		m_lineBuffer[m_lineCount].color.r = color.x();
		m_lineBuffer[m_lineCount].color.g = color.y();
		m_lineBuffer[m_lineCount].color.b = color.z();
		m_lineBuffer[m_lineCount + 1].color.r = color.x();
		m_lineBuffer[m_lineCount + 1].color.g = color.y();
		m_lineBuffer[m_lineCount + 1].color.b = color.z();

		m_lineCount += 2;
	}
}


void CollisionDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {

}

void CollisionDebugDraw::reportErrorWarning(const char* warningString) {

}

void CollisionDebugDraw::draw3dText(const btVector3& location, const char* textString) {

}

void CollisionDebugDraw::setDebugMode(int debugMode) {
	m_debugMode = debugMode;
}

int CollisionDebugDraw::getDebugMode() const {
	return m_debugMode;
}


void CollisionDebugDraw::clearLines() {
	m_lineCount = 0;
}