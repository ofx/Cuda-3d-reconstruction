#include "Stdafx.h"

#include "Camera.h"

Camera::Camera(glm::vec2 dimensions)
{
	// Defaults
	this->m_AspectRatio = 16.0f / 9.0f;
	this->m_Fov = 45.0f;
	this->m_Near = 0.1f;
	this->m_Far = 5000.0f;
	this->m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
	this->m_Position = glm::vec3(0.0f, 0.0f, -1000.0f);
	this->m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
	this->m_MouseSensitivity = 0.05f;
	this->m_ScrollSensitivity = 0.05f;
	this->m_MovementSenstivity = 100.0f;
	this->m_Yaw = -90.0f;
	this->m_Pitch = 0.0f;

	this->m_Dimensions = dimensions;
	this->m_LastX = dimensions.x * 0.5f;
	this->m_LastY = dimensions.y * 0.5f;

	this->DefineMatrices();
}

Camera::~Camera(void)
{
}

void Camera::DefineMatrices(void)
{
	this->m_Projection = glm::perspective(this->m_Fov, this->m_AspectRatio, this->m_Near, this->m_Far);
	this->m_View = glm::lookAt(this->m_Position, this->m_Position + this->m_Front, this->m_Up);
}

glm::mat4 Camera::GetView(void)
{
	return this->m_View;
}

glm::mat4 Camera::GetProjection(void)
{
	return this->m_Projection;
}

void Camera::SetFov(float fov)
{
	this->m_Fov = fov;

	this->DefineMatrices();
}

void Camera::SetAspectRatio(float aspectRatio)
{
	this->m_AspectRatio = aspectRatio;

	this->DefineMatrices();
}

void Camera::SetNear(float n)
{
	this->m_Near = n;

	this->DefineMatrices();
}

void Camera::SetFar(float f)
{
	this->m_Far = f;

	this->DefineMatrices();
}

void Camera::SetPosition(glm::vec3 position)
{
	this->m_Position = position;

	this->DefineMatrices();
}

void Camera::SetUp(glm::vec3 up)
{
	this->m_Up = up;

	this->DefineMatrices();
}

void Camera::SetDimensions(glm::vec2 dimensions)
{
	this->m_Dimensions = dimensions;
}

void Camera::SetSensitivity(float sensitivity)
{
	this->m_MouseSensitivity = sensitivity;
}

void Camera::HandleScroll(float s)
{
	if (this->m_Fov >= 1 && this->m_Fov <= 45.0f)
	{
		this->m_Fov += s * this->m_ScrollSensitivity;
	}
	
	if (this->m_Fov <= 1.0f)
	{
		this->m_Fov = 1.0f;
	}
	else if (this->m_Fov >= 45.0f)
	{
		this->m_Fov = 45.0f;
	}
	
	this->DefineMatrices();
}

bool b;
void Camera::HandleMouse(float x, float y)
{
	if (b)
    {
		this->m_LastX = x;
		this->m_LastY = y;
        b = false;
    }

    GLfloat xoffset = x - this->m_LastX;
    GLfloat yoffset = this->m_LastY - y;
    this->m_LastX = x; this->m_LastY = y;

    xoffset *= this->m_MouseSensitivity;
    yoffset *= this->m_MouseSensitivity;

    this->m_Yaw += xoffset;
    this->m_Pitch += yoffset;

    if (this->m_Pitch > 89.0f)
	{
        this->m_Pitch = 89.0f;
	}
    if (this->m_Pitch < -89.0f)
	{
        this->m_Pitch = -89.0f;
	}

    glm::vec3 front;
    front.x = cos(glm::radians(this->m_Yaw)) * cos(glm::radians(this->m_Pitch));
    front.y = sin(glm::radians(this->m_Pitch));
    front.z = sin(glm::radians(this->m_Yaw)) * cos(glm::radians(this->m_Pitch));
    this->m_Front = glm::normalize(front);

	this->DefineMatrices();
}

void Camera::HandleMovement(float dt, MovementDirection direction)
{
	GLfloat movementSpeed = this->m_MovementSenstivity * dt;

	switch (direction)
	{
	case FORWARD:
		this->m_Position += movementSpeed * this->m_Front;
		break;
	case BACKWARD:
		this->m_Position -= movementSpeed * this->m_Front;
		break;
	case LEFT:
		this->m_Position -= movementSpeed * glm::normalize(glm::cross(this->m_Front, this->m_Up));
		break;
	case RIGHT:
		this->m_Position += movementSpeed * glm::normalize(glm::cross(this->m_Front, this->m_Up));
		break;
	case UP:
		this->m_Position += movementSpeed * this->m_Up;
		break;
	case DOWN:
		this->m_Position -= movementSpeed * this->m_Up;
		break;
	}

	this->DefineMatrices();
}