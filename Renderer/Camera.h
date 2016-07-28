#pragma once

enum MovementDirection
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
private:
	glm::mat4 m_View, m_Projection;

	glm::vec3 m_Position;
	glm::vec3 m_Up;
	glm::vec3 m_Front;

	glm::vec2 m_Dimensions;

	float m_Yaw, m_Pitch;

	float m_ScrollSensitivity;
	float m_MouseSensitivity;
	float m_MovementSenstivity;

	float m_LastX, m_LastY;

	float m_Fov, m_AspectRatio, m_Near, m_Far;

	void DefineMatrices(void);
public:
	Camera(glm::vec2 dimensions);
	~Camera(void);

	glm::mat4 GetView(void);
	glm::mat4 GetProjection(void);

	void SetFov(float fov);
	void SetAspectRatio(float aspectRatio);
	void SetNear(float near);
	void SetFar(float far);
	void SetPosition(glm::vec3 position);
	void SetUp(glm::vec3 up);
	void SetDimensions(glm::vec2 dimensions);
	void SetSensitivity(float sensitivity);

	void HandleScroll(float s);
	void HandleMouse(float x, float y);
	void HandleMovement(float dt, MovementDirection direction);
};

