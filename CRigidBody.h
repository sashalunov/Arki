// --------------------------------------------------------------------------------
// 	file: CRigidBody.h
//---------------------------------------------------------------------------------
//  created: 20.12.2025 18:23 by JIyHb
// --------------------------------------------------------------------------------

#pragma once
#include "CArkiBlock.h"

#include <btBulletDynamicsCommon.h>

enum RigidBodyType {
    RB_BOX,
    RB_BALL,
    RB_CAPSULE,
    RB_ROD,
};

//--------------------------------------------------------------------------------------
class CRigidBody : public btMotionState
{
public:
    bool         m_isSelected;
	bool 	 m_isVisible;
	RigidBodyType m_type;
    std::string  m_name;

    // Transforms
    D3DXVECTOR3    m_position;
    D3DXQUATERNION m_rotation;
    D3DXVECTOR3    m_scale;
    D3DXVECTOR3    m_eulerAngles;
    // Physics
    btRigidBody* m_rigidBody;
    btCollisionShape* m_shape;
    btDynamicsWorld* m_world;
    bool              m_ownsShape;
    PhysicsData* m_pPhysicsData; // Store pointer for cleanup

    // --- SHARED RESOURCES (STATIC) ---
// All blocks share this one mesh to save memory
    static ID3DXMesh* s_pRigidBodyBoxMesh;
    static ID3DXMesh* s_pRigidBodySphereMesh;
    static ID3DXMesh* s_pRigidBodyCapsuleMesh;
    static ID3DXMesh* s_pRigidBodyCylinderMesh;

public:
    CRigidBody();
    virtual ~CRigidBody() { DestroyPhysics(); }

    // --- 1. SETUP: Now with Active/Passive Control ---
    // isDynamic = false (Default): Mass is forced to 0. Object is STATIC.
    // isDynamic = true:            Mass is used. Object is ACTIVE.
    void InitPhysics(btDynamicsWorld* world, btCollisionShape* shape, float mass, bool isDynamic , bool ownsShape );
    // Helper: Create a box (Passive by default)
    void InitBox(btDynamicsWorld* world, D3DXVECTOR3 size, float mass, bool isDynamic );
    // 2. SPHERE: simple radius
    void InitSphere(btDynamicsWorld* world, float radius, float mass, bool isDynamic);
    // 3. CAPSULE: Good for characters. Aligned on Y-axis.
    // Note: 'height' is the height of the cylinder part only. Total height = height + 2 * radius.
    void InitCapsule(btDynamicsWorld* world, float radius, float height, float mass, bool isDynamic);
    // 4. CYLINDER: Aligned on Y-axis.
    void InitCylinder(btDynamicsWorld* world, float radius, float height, float mass, bool isDynamic);
    // --- 2. CLEANUP ---
    void DestroyPhysics();

    // --- Naming Functions ---
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    // 1. Static Setup (Call this ONCE in Game::Init)
    static HRESULT InitSharedMesh(IDirect3DDevice9* device)
    {
		HRESULT hr = S_OK;
        if (!s_pRigidBodyBoxMesh) 
            hr = D3DXCreateBox(device, 1.0f, 1.0f, 1.0f, &s_pRigidBodyBoxMesh, NULL);

        if(!s_pRigidBodySphereMesh)
			hr = D3DXCreateSphere(device, 0.5f, 32, 16, &s_pRigidBodySphereMesh, NULL);
        
		if (!s_pRigidBodyCapsuleMesh)
            hr = D3DXCreateCylinder(device, 0.5f, 0.5f, 1.0f, 16, 8, &s_pRigidBodyCapsuleMesh, NULL);

		if (!s_pRigidBodyCylinderMesh)
			hr = D3DXCreateCylinder(device, 0.5f, 0.5f, 1.0f, 16, 8, &s_pRigidBodyCylinderMesh, NULL);
        // Create a 1x1x1 Unit Cube
        // We will scale this up to the correct size in Render()
		return hr;
    }
    // 2. Static Cleanup (Call this ONCE in Game::Shutdown)
    static void CleanupSharedMesh()
    {
		SAFE_RELEASE(s_pRigidBodyBoxMesh);
		SAFE_RELEASE(s_pRigidBodySphereMesh);
		SAFE_RELEASE(s_pRigidBodyCapsuleMesh);
		SAFE_RELEASE(s_pRigidBodyCylinderMesh);
    }

    // --- 3. RENDER & BULLET INTERFACE ---
    D3DXMATRIXA16 GetWorldMatrix()
    {
        D3DXMATRIXA16 matScale, matRot, matTrans;
        D3DXMatrixScaling(&matScale, m_scale.x, m_scale.y, m_scale.z);
        D3DXMatrixRotationQuaternion(&matRot, &m_rotation);
        D3DXMatrixTranslation(&matTrans, m_position.x, m_position.y, m_position.z);
        return matScale * matRot * matTrans;
    }

    virtual void getWorldTransform(btTransform& worldTrans) const override
    {
        worldTrans.setIdentity();
        worldTrans.setOrigin(btVector3(m_position.x, m_position.y, m_position.z));
        worldTrans.setRotation(btQuaternion(m_rotation.x, m_rotation.y, m_rotation.z, m_rotation.w));
    }

    virtual void setWorldTransform(const btTransform& worldTrans) override
    {
        btVector3 pos = worldTrans.getOrigin();
        m_position = D3DXVECTOR3((const FLOAT)pos.x(), (const FLOAT)pos.y(), (const FLOAT)pos.z());
        btQuaternion rot = worldTrans.getRotation();
        m_rotation = D3DXQUATERNION(static_cast<float>(rot.x()), static_cast<float>(rot.y()), static_cast<float>(rot.z()), static_cast<float>(rot.w()));
    }

    // USE THIS to move objects, do not set m_position directly!
    void SetPosition(float x, float y, float z)
    {
        // 1. Update CPU (Graphics)
        m_position = D3DXVECTOR3(x, y, z);

        // 2. Update Bullet (Physics)
        if (m_rigidBody)
        {
            btTransform trans = m_rigidBody->getWorldTransform();
            trans.setOrigin(btVector3(x, y, z));
            m_rigidBody->setWorldTransform(trans);

            // Important: If the object was sleeping (static), wake it up!
            m_rigidBody->activate();

            // Optional: If it is a Static object (Mass 0), we sometimes need to 
            // tell the broadphase to update immediately, or collisions might look wrong for one frame.
            if (m_world) m_world->updateSingleAabb(m_rigidBody);
        }
    }
    // USE THIS to move objects, do not set m_position directly!
    void SetPosition(btVector3 vpos)
    {
        // 1. Update CPU (Graphics)
        m_position = D3DXVECTOR3(
            static_cast<float>(vpos.x()), 
            static_cast<float>(vpos.y()), 
            static_cast<float>(vpos.z()));

        // 2. Update Bullet (Physics)
        if (m_rigidBody)
        {
            btTransform trans = m_rigidBody->getWorldTransform();
            trans.setOrigin(vpos);
            m_rigidBody->setWorldTransform(trans);

            // Important: If the object was sleeping (static), wake it up!
            m_rigidBody->activate();

            // Optional: If it is a Static object (Mass 0), we sometimes need to 
            // tell the broadphase to update immediately, or collisions might look wrong for one frame.
            if (m_world) m_world->updateSingleAabb(m_rigidBody);
        }
    }
    // USE THIS to rotate objects
    void SetRotation(float yaw, float pitch, float roll)
    {
        m_eulerAngles = D3DXVECTOR3(yaw, pitch, roll);
        // 1. Update CPU
        D3DXQuaternionRotationYawPitchRoll(&m_rotation, yaw, pitch, roll);

        // 2. Update Bullet
        if (m_rigidBody)
        {
            btTransform trans = m_rigidBody->getWorldTransform();
            btQuaternion quat(m_rotation.x, m_rotation.y, m_rotation.z, m_rotation.w);
            trans.setRotation(quat);
            m_rigidBody->setWorldTransform(trans);

            m_rigidBody->activate();
            if (m_world) m_world->updateSingleAabb(m_rigidBody);
        }
    }

    btVector3 GetPosition() const
    {
        return m_rigidBody->getWorldTransform().getOrigin();
    }

    // Get AABB for the Bounding Box
    void GetAabb(btVector3& min, btVector3& max)
    {
        m_rigidBody->getAabb(min, max);
    }

    void Render(IDirect3DDevice9* device)
    {
        D3DXMATRIXA16 worldMat, matRot, matScale;

        if (m_isVisible)
        {
            worldMat = GetWorldMatrix();
            device->SetTransform(D3DTS_WORLD, &worldMat);
			device->SetRenderState(D3DRS_LIGHTING, TRUE);
            device->SetTexture(0, NULL);

            // Simple white material
            static D3DMATERIAL9 WHITE_MTRL =
            {

                1.0f, 1.0f, 1.0f, 1.0f , // Diffuse
             0.8f, 0.8f, 0.8f, 1.0f , // Ambient
             0.0f, 0.0f, 0.0f, 1.0f , // Specular
             0.0f, 0.0f, 0.0f, 1.0f , // Emissive
            0.0f };                        // Power

            device->SetMaterial(&WHITE_MTRL);

            switch (m_type)
            {
            case RB_BOX:
                s_pRigidBodyBoxMesh->DrawSubset(0);
                break;
            case RB_BALL:
                //device->SetMaterial(&WHITE_MTRL);
                s_pRigidBodySphereMesh->DrawSubset(0);
                break;
            case RB_CAPSULE:
                //device->SetMaterial(&WHITE_MTRL);
                s_pRigidBodyCapsuleMesh->DrawSubset(0);
                break;
            case RB_ROD:
                //device->SetMaterial(&WHITE_MTRL);
                s_pRigidBodyCylinderMesh->DrawSubset(0);
                break;
            default:
                break;
            }
        }
        // Render Selection Box if selected
        if (m_isSelected)
        {
            RenderBoundingBox(device);
        }

		D3DXMatrixIdentity(&worldMat);
		device->SetTransform(D3DTS_WORLD, &worldMat);
	}

    void RenderBoundingBox(IDirect3DDevice9* device)
    {
        btVector3 min, max;
        GetAabb(min, max);

        // Calculate 8 corners
        D3DXVECTOR3 corners[8];
        corners[0] = D3DXVECTOR3(min.x(), min.y(), min.z());
        corners[1] = D3DXVECTOR3(max.x(), min.y(), min.z());
        corners[2] = D3DXVECTOR3(min.x(), max.y(), min.z());
        corners[3] = D3DXVECTOR3(max.x(), max.y(), min.z());
        corners[4] = D3DXVECTOR3(min.x(), min.y(), max.z());
        corners[5] = D3DXVECTOR3(max.x(), min.y(), max.z());
        corners[6] = D3DXVECTOR3(min.x(), max.y(), max.z());
        corners[7] = D3DXVECTOR3(max.x(), max.y(), max.z());

        // Indices for lines
        short indices[] = {
            0,1, 1,3, 3,2, 2,0, // Front face
            4,5, 5,7, 7,6, 6,4, // Back face
            0,4, 1,5, 2,6, 3,7  // Connecting lines
        };

        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        device->SetTransform(D3DTS_WORLD, &identity); // Draw in world space
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetFVF(D3DFVF_XYZ);
        device->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, 8, 12, indices, D3DFMT_INDEX16, corners, sizeof(D3DXVECTOR3));
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
    }

    void RescaleObject( btVector3 newScale)
    {
        if (!m_rigidBody || !m_rigidBody->getCollisionShape()) return;

        // 1. Apply the new scale to the shape
        // Note: This is absolute, not relative. (1,1,1) resets to original size.
        m_rigidBody->getCollisionShape()->setLocalScaling(newScale/2.0f);
        m_scale = D3DXVECTOR3(newScale.x(), newScale.y(), newScale.z());
        // 2. Recalculate inertia (Only needed for Dynamic objects like the Ball)
        // For the Paddle (Kinematic), this step can be skipped, but it's safe to include.
        if (!m_rigidBody->isStaticOrKinematicObject())
        {
            btScalar mass = 1.0f / m_rigidBody->getInvMass(); // Recover mass
            btVector3 localInertia;
            m_rigidBody->getCollisionShape()->calculateLocalInertia(mass, localInertia);
            m_rigidBody->setMassProps(mass, localInertia);
        }

        // 3. CRITICAL: Tell the world the object size changed
        // If you skip this, the broadphase won't update, and the object will clip/pass through things.
        m_world->updateSingleAabb(m_rigidBody);

        // 4. Wake the object up so it interacts immediately
        m_rigidBody->activate(true);
    }
};