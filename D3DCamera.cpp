#include "StdAfx.h"
#include "btBulletCollisionCommon.h"
#include "D3DCamera.h"
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CD3DCamera::CD3DCamera(LPDIRECT3DDEVICE9 dev, float fov )
{
	D3DXVECTOR3 vEye = D3DXVECTOR3(0.0f, 20.0f, -10.0f);
	D3DXVECTOR3 vAt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    // Update the view matrix
	SetView(&vEye, &vAt);
	SetProj(fov);

	m_fMovSpeed = 0.3f;
	m_fRotSpeed = 0.1f;

	m_pD3DDev = dev;

	m_qOrientation = m_qOrientation.getIdentity();
	m_matView.setIdentity();

	m_fYawAngle = 0;
	m_fPitchAngle = 0;
	m_fRollAngle = 0;

}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CD3DCamera::~CD3DCamera(void)
{
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CD3DCamera::SetView(D3DXVECTOR3* pvEyePt, D3DXVECTOR3* pvLookPt)
{
    if( NULL == pvEyePt || NULL == pvLookPt )
        return;

	m_vecEye	= (btVector3&)pvEyePt;
	m_vecLook	= (btVector3&)pvLookPt;

	D3DXVECTOR3 vUp( 0,1,0 );
    D3DXMatrixLookAtLH( &m_pView, pvEyePt, pvLookPt, &vUp );

}

void CD3DCamera::SetProj(float fov , float aspect)
{
	m_fFov		= D3DXToRadian(fov);
	m_fAspect	= aspect;
	m_fNear		= 0.1f;
	m_fFar		= 256.0f;

	D3DXMatrixPerspectiveFovLH(&m_pProj, m_fFov, m_fAspect, m_fNear, m_fFar);

}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CD3DCamera::Update(void)
{
	D3DXMATRIX tempview, tempproj;

	btQuaternion qPitch	= btQuaternion( btVector3(1,0,0), btRadians(m_fPitchAngle));
	btQuaternion qYaw	= btQuaternion( btVector3(0,1,0), btRadians(m_fYawAngle));
	btQuaternion qRoll	= btQuaternion( btVector3(0,0,1), btRadians(m_fRollAngle));

	m_qOrientation = qYaw * qPitch *  qRoll;

	m_matView.setIdentity();
	m_matView.setRotation(m_qOrientation.inverse());
	//m_matView.setOrigin( m_matView( m_vecEye )  );


	////m_matView.getOpenGLMatrix(tempview);
	//m_matProj.getOpenGLMatrix(tempproj);

	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
	D3DXMatrixMultiply(&m_pViewProj, &m_pView, &m_pProj);

	m_pD3DDev->SetTransform(D3DTS_VIEW, &m_pView);
	m_pD3DDev->SetTransform(D3DTS_PROJECTION, &tempproj);

	//m_pView = tempview;
	//m_pProj = tempproj;

	D3DXMatrixInverse( &m_mCameraWorld, NULL, &m_pView );

}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveForward(float fTime)
{
	//btVector3 axis = btVector3(0, 0, (m_fMovSpeed * fTime));
	//btVector3 trans = quatRotate(m_qOrientation, axis);
    //m_vecEye -= axis;

	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, 0, 0, -(m_fMovSpeed * fTime) );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveBackward(float fTime)
{
	//btVector3 axis = btVector3(0, 0, (m_fMovSpeed * fTime));
	//btVector3 trans = quatRotate(m_qOrientation, axis);
	//m_vecEye += axis;
	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, 0, 0, (m_fMovSpeed * fTime) );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveLeft(float fTime)
{
	//btVector3 axis = btVector3((m_fMovSpeed * fTime), 0, 0);
	//btVector3 trans = quatRotate(m_qOrientation, axis);
    //m_vecEye += axis;
	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, (m_fMovSpeed * fTime), 0, 0 );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveRight(float fTime)
{
	//btVector3 axis = btVector3((m_fMovSpeed * fTime), 0, 0);
	//btVector3 trans = quatRotate(m_qOrientation, axis);
    //m_vecEye -= axis;
	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, -(m_fMovSpeed * fTime), 0, 0 );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveUp(float fTime)
{
	//btVector3 axis = btVector3(0, (m_fMovSpeed * fTime), 0);
	//btVector3 trans = quatRotate(m_qOrientation, axis);
    //m_vecEye -= axis;
	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, 0, -(m_fMovSpeed * fTime), 0 );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::MoveDown(float fTime)
{
	//btVector3 axis = btVector3(0, (m_fMovSpeed * fTime), 0);
	//btVector3 trans = quatRotate(m_qOrientation, axis);
    //m_vecEye += axis;
	D3DXMATRIX tempview;

	D3DXMatrixIdentity(&tempview);
	D3DXMatrixTranslation(&tempview, 0, (m_fMovSpeed * fTime), 0 );
	D3DXMatrixMultiply(&m_pView, &m_pView, &tempview);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::Pitch(float theta)
{
	m_fPitchAngle = theta * m_fRotSpeed;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::Yaw(float theta)
{
	m_fYawAngle = theta * m_fRotSpeed;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CD3DCamera::Roll(float theta)
{
	m_fRollAngle = theta * m_fRotSpeed;
}

