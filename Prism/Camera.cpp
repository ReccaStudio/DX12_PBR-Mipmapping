#include "Camera.h"

//using namespace DirectX;
//
//Camera::Camera(uint32_t a_ScreenWidth, uint32_t a_ScreenHeight, float a_ScreenDepth, float a_ScreenNear)
//{
//	m_ScreenWidth = a_ScreenWidth;
//	m_ScreenHeight = a_ScreenHeight;
//	m_ScreenNear = a_ScreenNear;
//	m_ScreenFar = a_ScreenDepth;
//
//	const float fieldOfView = XM_PI / 4.0f;
//
//	const float screenAspect = (float)m_ScreenWidth / (float)m_ScreenHeight;
//
//	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, m_ScreenNear, m_ScreenFar);
//
//	m_Position = XMFLOAT3(0.0f, 0.0f, -40.0f);
//	m_Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
//	m_PrevRotationQuat = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
//
//	UpdateViewMatrix();
//}
//
//void Camera::AddRotation(float x, float y, float z)
//{
//	m_Rotation.x -= x;
//	m_Rotation.y -= y;
//	m_Rotation.z -= z;
//}
//
//void Camera::Update()
//{
//	UpdateViewMatrix();
//}
//
//void Camera::UpdateViewMatrix()
//{
//	XMFLOAT3 up, lookAt;
//	XMVECTOR upVector, positionVector, lookAtVector;
//	XMMATRIX rotationMatrix;
//
//	// Setup the vector that points upwards.
//	up.x = 0.0f;
//	up.y = 1.0f;
//	up.z = 0.0f;
//
//	// Load it into a XMVECTOR structure.
//	upVector = XMLoadFloat3(&up);
//
//	// Load it into a XMVECTOR structure.
//	positionVector = XMLoadFloat3(&m_Position);
//
//	// Setup where the camera is looking by default.
//	lookAt.x = 0.0f;
//	lookAt.y = 0.0f;
//	lookAt.z = 1.0f;
//
//	// Load it into a XMVECTOR structure.
//	lookAtVector = XMLoadFloat3(&lookAt);
//
//	// We slerp the rotation to avoid stuttery camera rotations
//	XMVECTOR currRotQuat = m_PrevRotationQuat;
//
//	XMVECTOR wantedRotQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Rotation.x),
//		XMConvertToRadians(m_Rotation.y),
//		XMConvertToRadians(m_Rotation.z));
//
//	//TODO Change magic value with delta time
//	XMVECTOR slerpedRot = XMQuaternionSlerp(currRotQuat, wantedRotQuat, 0.05f);
//
//	m_PrevRotationQuat = slerpedRot;
//
//	// Create the rotation matrix from the yaw, pitch, and roll values.
//	rotationMatrix = XMMatrixRotationQuaternion(slerpedRot);
//
//	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
//	lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
//	upVector = XMVector3TransformCoord(upVector, rotationMatrix);
//
//	// Translate the rotated camera position to the location of the viewer.
//	lookAtVector = XMVectorAdd(positionVector, lookAtVector);
//
//	// Finally create the view matrix from the three updated vectors.
//	m_ViewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
//}
//
//XMMATRIX Camera::GetViewMatrix() const
//{
//	return m_ViewMatrix;
//}
//
//XMMATRIX Camera::GetProjectionMatrix() const
//{
//	return m_ProjectionMatrix;
//}
//
//void Camera::AddForwardMovement(float a_Amount)
//{
//	XMFLOAT4X4 toFloatViewMatrix;
//
//	XMStoreFloat4x4(&toFloatViewMatrix, m_ViewMatrix);
//
//	XMFLOAT3 forward = XMFLOAT3(toFloatViewMatrix._13, toFloatViewMatrix._23, toFloatViewMatrix._33);
//
//	m_Position.x += forward.x * a_Amount;
//	m_Position.y += forward.y * a_Amount;
//	m_Position.z += forward.z * a_Amount;
//}
//
//void Camera::AddSideMovement(float a_Amount)
//{
//	XMFLOAT4X4 toFloatViewMatrix;
//
//	XMStoreFloat4x4(&toFloatViewMatrix, m_ViewMatrix);
//
//	XMFLOAT3 side = XMFLOAT3(toFloatViewMatrix._11, toFloatViewMatrix._21, toFloatViewMatrix._31);
//
//	m_Position.x += side.x * a_Amount;
//	m_Position.y += side.y * a_Amount;
//	m_Position.z += side.z * a_Amount;
//}
//
//void Camera::AddUpMovement(float a_Amount)
//{
//	XMFLOAT4X4 toFloatViewMatrix;
//
//	XMStoreFloat4x4(&toFloatViewMatrix, m_ViewMatrix);
//
//	XMFLOAT3 up = XMFLOAT3(toFloatViewMatrix._12, toFloatViewMatrix._22, toFloatViewMatrix._32);
//
//	m_Position.x += up.x * a_Amount;
//	m_Position.y += up.y * a_Amount;
//	m_Position.z += up.z * a_Amount;
//}
