#pragma once
#include "DirectX12Headers.h"

enum Camera_Movement {
	FORWARD = 0u,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 20.0f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0f;

using namespace DirectX;

class Camera
{
public:
	// Camera Attributes
	DirectX::XMVECTOR Position;
	DirectX::XMVECTOR Front;
	DirectX::XMVECTOR Up;
	DirectX::XMVECTOR Right;
	DirectX::XMVECTOR WorldUp;
	DirectX::XMMATRIX Projection;

	// Euler Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// Constructor with vectors
	Camera(DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3 up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) 
		: Front(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = DirectX::XMLoadFloat3(&position);
		DirectX::XMVectorSetW(Position, 1.0f);

		WorldUp = DirectX::XMLoadFloat3(&up);
		DirectX::XMVectorSetW(WorldUp, 0.0f);

		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = DirectX::XMVectorSet(posX, posY, posZ, 1.0f);
		WorldUp = DirectX::XMVectorSet(upX, upY, upZ, 0.0f);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	DirectX::XMFLOAT3 GetPosition()
	{
		DirectX::XMFLOAT3 pos;
		XMStoreFloat3(&pos, Position);

		return pos;
	}

	// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	DirectX::XMMATRIX GetViewMatrix()
	{
		return DirectX::XMMatrixLookAtRH(Position, Position + Front, Up);
	}

	void SetProjectionMatrix(DirectX::XMMATRIX a_Projection)
	{
		Projection = a_Projection;
	}

	DirectX::XMMATRIX& GetProjectionMatrix()
	{
		return Projection;
	}

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position -= Right * velocity;
		if (direction == RIGHT)
			Position += Right * velocity;
		if (direction == UP)
			Position += Up * velocity;
		if (direction == DOWN)
			Position -= Up * velocity;
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

private:
	// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		DirectX::XMFLOAT4 front;
		front.x = cos(DirectX::XMConvertToRadians(Yaw)) * cos(DirectX::XMConvertToRadians(Pitch));
		front.y = sin(DirectX::XMConvertToRadians(Pitch));
		front.z = sin(DirectX::XMConvertToRadians(Yaw)) * cos(DirectX::XMConvertToRadians(Pitch));
		front.w = 0.0f;

		DirectX::XMVECTOR frontVec = XMLoadFloat4(&front);

		Front = DirectX::XMVector4Normalize(frontVec);

		// Also re-calculate the Right and Up vector
		Right = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(Right, Front));
	}
};

/*
class Camera
{
public:

	Camera(uint32_t a_ScreenWidth, uint32_t a_ScreenHeight, float a_ScreenDepth, float a_ScreenNear);
	Camera(const Camera& a_Rhs) = delete;
	virtual ~Camera() {}

	void AddRotation(float a_X, float a_Y, float a_Z);

	virtual void Update();

	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix() const;
	DirectX::XMFLOAT3 GetPosition() { return m_Position; }

	void AddForwardMovement(float a_Amount);
	void AddSideMovement(float a_Amount);
	void AddUpMovement(float a_Amount);

private:

	void UpdateViewMatrix();

private:

	uint32_t m_ScreenWidth;
	uint32_t m_ScreenHeight;
	float m_ScreenNear;
	float m_ScreenFar;

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Rotation;
	DirectX::XMVECTOR m_PrevRotationQuat;

	mutable DirectX::XMMATRIX m_ViewMatrix;
	mutable DirectX::XMMATRIX m_ProjectionMatrix;
};
*/