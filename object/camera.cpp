#include "camera.h"
#include "../tools/input.h"

#undef min
#undef max

Camera::Camera(float fov, float aspect, float nearZ, float farZ) :
	m_viewMatrix(glm::identity<glm::mat4>()),
	m_fov(fov),
	m_aspect(aspect),
	m_near(nearZ),
	m_far(farZ)
{
	m_projMatrix = glm::perspective(fov, aspect, nearZ, farZ);

	m_frustumMin.resize(FRUSTUM_DIV_SIZE_X * FRUSTUM_DIV_SIZE_Y * FRUSTUM_DIV_SIZE_Z);
	m_frustumMax.resize(FRUSTUM_DIV_SIZE_X * FRUSTUM_DIV_SIZE_Y * FRUSTUM_DIV_SIZE_Z);
	for (int x = 0; x < FRUSTUM_DIV_SIZE_Z; x++) {
		for (int y = 0; y < FRUSTUM_DIV_SIZE_Y; y++) {
			for (int z = 0; z < FRUSTUM_DIV_SIZE_Z; z++) {
				int cellIndex = z * FRUSTUM_DIV_SIZE_Z * FRUSTUM_DIV_SIZE_Y + y * FRUSTUM_DIV_SIZE_Y + x;
				glm::vec4 Min =
					glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) +
					glm::vec4((float)x / (float)FRUSTUM_DIV_SIZE_X,
						(float)y / (float)FRUSTUM_DIV_SIZE_Y,
						(float)z / (float)FRUSTUM_DIV_SIZE_Z,
						0.0f);

				glm::vec4 Max =
					glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) +
					glm::vec4((float)(x + 1) / (float)FRUSTUM_DIV_SIZE_X,
						(float)(y + 1) / (float)FRUSTUM_DIV_SIZE_Y,
						(float)(z + 1) / (float)FRUSTUM_DIV_SIZE_Z,
						0.0f);

				Min.x = Min.x * 2.0f - 1.0f;
				Min.y = Min.y * 2.0f - 1.0f;
				Max.x = Max.x * 2.0f - 1.0f;
				Max.y = Max.y * 2.0f - 1.0f;

				Min.z = Max.z = 1.0f;

				Min = glm::inverse(m_projMatrix) * Min;
				Max = glm::inverse(m_projMatrix) * Max;
				Min /= Min.w;
				Max /= Max.w;

				Min.z = -Min.z;
				Max.z = -Max.z;

				float nearPlane = (farZ) / (float)FRUSTUM_DIV_SIZE_Z * (float)z + nearZ;
				float farPlane = (farZ) / (float)FRUSTUM_DIV_SIZE_Z * (float)(z + 1) + nearZ;

				glm::vec3 minPointNear = lineIntersectionZPlane(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(Min.x, Min.y, Min.z), nearPlane);
				glm::vec3 minPointFar = lineIntersectionZPlane(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(Min.x, Min.y, Min.z), farPlane);
				glm::vec3 maxPointNear = lineIntersectionZPlane(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(Max.x, Max.y, Max.z), nearPlane);
				glm::vec3 maxPointFar = lineIntersectionZPlane(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(Max.x, Max.y, Max.z), farPlane);
				

				glm::vec3 minPointAABB = glm::min(glm::min(minPointNear, minPointFar), glm::min(maxPointNear, maxPointFar));
				glm::vec3 maxPointAABB = glm::max(glm::max(minPointNear, minPointFar), glm::max(maxPointNear, maxPointFar));

				m_frustumMin[cellIndex] = glm::vec4(minPointAABB, 0.0f);
				m_frustumMax[cellIndex] = glm::vec4(maxPointAABB, 0.0f);
			}
		}
	}

	int a = 0;
}

Camera::~Camera() {

}

void Camera::Update(float deltaTime) {
	if (m_transform.parent() == nullptr) {
		if (Input::Instance().Push(DIK_S)) {
			m_transform.AddTranslation(-m_transform.forward().x, -m_transform.forward().y, -m_transform.forward().z);
		}
		if (Input::Instance().Push(DIK_W)) {
			m_transform.AddTranslation(m_transform.forward().x, m_transform.forward().y, m_transform.forward().z);
		}
		if (Input::Instance().Push(DIK_A)) {
			m_transform.AddTranslation(-m_transform.right().x, -m_transform.right().y, -m_transform.right().z);
		}
		if (Input::Instance().Push(DIK_D)) {
			m_transform.AddTranslation(m_transform.right().x, m_transform.right().y, m_transform.right().z);
		}
		if (Input::Instance().Push(DIK_E)) {
			m_transform.AddTranslation(m_transform.up().x, m_transform.up().y, m_transform.up().z);
		}
		if (Input::Instance().Push(DIK_Q)) {
			m_transform.AddTranslation(-m_transform.up().x, -m_transform.up().y, -m_transform.up().z);
		}
	}

	static float pitch, yaw;

	pitch -= (float)Input::Instance().GetMoveYRightPushed() * 0.1f * deltaTime;
	yaw += (float)Input::Instance().GetMoveXRightPushed() * 0.1f * deltaTime;
	
	//m_transform.RotateLocal(0.0f, 0.0f, (float)Input::Instance().GetMoveYRightPushed() * 0.01f);
	//m_transform.RotateLocal((float)Input::Instance().GetMoveYRightPushed() * 0.01f, (float)Input::Instance().GetMoveXRightPushed() * 0.01f, 0.0f);

	m_transform.localRotation() =
		glm::rotate(glm::identity<glm::quat>(), yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::identity<glm::quat>(), pitch, glm::vec3(0.0f, 0.0f, 1.0f));
	m_transform.SetDirtyFlag();
	m_transform.DirtyUpdate();

	static float range = 10.0f;

	if (Input::Instance().Push(DIK_LSHIFT))
		range += 100.1f * deltaTime;
	if (Input::Instance().Push(DIK_LCONTROL))
		range -= 100.1f * deltaTime;
	
	glm::vec3 position = m_transform.position();
	if (m_transform.parent()) {
		position = m_transform.parent()->position();
	}

	m_viewMatrix = glm::lookAt(position - m_transform.forward() * range, position, m_transform.up());
	m_projMatrix = glm::perspective(m_fov, m_aspect, m_near, m_far);
}


glm::vec3 Camera::lineIntersectionZPlane(glm::vec3 A, glm::vec3 B, float zPlane) {
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::vec3 ab = B - A;
	
	float t = (zPlane - glm::dot(normal, A)) / glm::dot(normal, ab);
	
	glm::vec3 result = A + t * ab;

	return result;
}