// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

#pragma once
#include "btBulletCollisionCommon.h"
#include <btBulletDynamicsCommon.h>



// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
class CD3DCamera
{
public:
	CD3DCamera(LPDIRECT3DDEVICE9 dev, float fov = 90.0f);
	~CD3DCamera(void);
	void Update(void);

	const D3DXVECTOR3* GetWorldRight() const { return (D3DXVECTOR3*)&m_mCameraWorld._11; } 
    const D3DXVECTOR3* GetWorldUp() const    { return (D3DXVECTOR3*)&m_mCameraWorld._21; }
    const D3DXVECTOR3* GetWorldAhead() const { return (D3DXVECTOR3*)&m_mCameraWorld._31; }
    const D3DXVECTOR3* GetEyePt() const      { return (D3DXVECTOR3*)&m_mCameraWorld._41; }

	void SetView(D3DXVECTOR3* pvEyePt, D3DXVECTOR3* pvLookPt);
	void SetProj(float fov = 90.0f, float aspect = 1.0f);

	btVector3 GetPos(void) const { return m_vecEye; }
	btVector3 GetLook(void) const { return m_vecLook; }
	btVector3 GetUp(void) const { return m_vecUp; }
	btQuaternion GetRot(void) const { return m_qOrientation; }
	D3DXMATRIX GetViewProj(void) const {return m_pViewProj;}
	D3DXMATRIX GetProj(void) const {return m_pProj;}
	D3DXMATRIX GetView(void) const {return m_pView;}

	float GetFov(void) const { return m_fFov; }
	//void SetFov(float fov) { m_fFov = fov; }
	float GetAspect(void) const { return m_fAspect; }
	//void SetAspect(float aspect) { m_fAspect = aspect; }

	void MoveForward(float fTime);
	void MoveBackward(float fTime);
	void MoveLeft(float fTime);
	void MoveRight(float fTime);
	void MoveUp(float fTime);
	void MoveDown(float fTime);

	void Pitch(float theta);
	void Yaw(float theta);
	void Roll(float theta);


protected:
	D3DXMATRIX m_mCameraWorld;       // World matrix of the camera (inverse of the view matrix)
	D3DXMATRIX m_pViewProj;
	D3DXMATRIX m_pView;
	D3DXMATRIX m_pProj;
	LPDIRECT3DDEVICE9	m_pD3DDev;

	float	m_fFov;
	float	m_fAspect;
	float	m_fNear;
	float	m_fFar;

	float	m_fMovSpeed;
	float	m_fRotSpeed;

	float	m_fYawAngle;
	float	m_fPitchAngle;
	float	m_fRollAngle;

	btQuaternion	m_qOrientation;
	btVector3		m_vecEye;
	btVector3		m_vecLook;
	btVector3		m_vecUp;
	btTransform		m_matView;

};

