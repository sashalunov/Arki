#pragma once
#include "CArkiBlock.h"

// The Vertex Format for the ribbon geometry
struct TRAILVERTEX
{
    D3DXVECTOR3 position; // The position in space
    D3DCOLOR    color;    // For fading alpha
    D3DXVECTOR2 tex;      // Texture coordinates (U, V)

    // FVF (Flexible Vertex Format) description
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};

class CArkiBall
{
private:
    btDiscreteDynamicsWorld* m_pDynamicsWorld; // Reference to physics world

	// --- GHOST TRAIL MEMBERS ---
    std::deque<D3DXVECTOR3> m_trailHistory;
    int m_maxTrailLength;
    float m_sampleTimer; // To control how often we record a position

    // --- RIBBON TRAIL MEMBERS ---
    std::deque<D3DXVECTOR3> m_pathHistory; // Stores center positions
    IDirect3DVertexBuffer9* m_pTrailVB;    // Dynamic buffer for geometry
    IDirect3DTexture9* m_pTrailTexture; // The gradient texture

    int   m_maxPathSegments; // How long the tail is
    float m_ribbonWidth;     // How wide the strip is
    float m_minPointDistSq;  // Optimization: don't record tiny movements

public:
    btRigidBody* m_pBody;
    float m_radius;
    float m_targetSpeed; // The speed we want to maintain constant
    bool  m_isActive;
    float m_lifetime;

    ID3DXMesh* m_pMesh;

    CArkiBall(btDiscreteDynamicsWorld* dynamicsWorld, IDirect3DDevice9* device, D3DXVECTOR3 startPos, float radius, float speed)
        : m_pDynamicsWorld(dynamicsWorld)
    {
        m_radius = radius;
        m_targetSpeed = speed;
        m_pMesh = NULL;
        m_isActive = false;
        m_lifetime = -1.0f; //infinite lifetime
        // 1. Create Sphere Shape
        btCollisionShape* shape = new btSphereShape(radius);
        D3DXCreateSphere(device, radius, 64, 48, &m_pMesh, NULL);
        // 2. Setup Mass and Inertia (Dynamic object, so mass > 0)
        btScalar mass = 1.0f;
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);

        // 3. Transform (Start Position)
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(startPos.x, startPos.y, startPos.z));
        btDefaultMotionState* motionState = new btDefaultMotionState(transform);

        // 4. Create Rigid Body
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
        m_pBody = new btRigidBody(rbInfo);

        // --- THE ARCANOID PHYSICS SETUP ---
        // Bounciness: 1.0 = 100% energy conservation on bounce
        m_pBody->setRestitution(1.0f);
        // Friction: 0.0 = No slowing down when sliding against walls/paddle
        m_pBody->setFriction(0.0f);
        m_pBody->setRollingFriction(0.0f);
        // Damping: 0.0 = No air resistance (linear or angular)
        m_pBody->setDamping(0.0f, 0.0f);
        // 2D Constraint: Lock Z axis so ball doesn't fly towards/away from camera
        // (Assuming gameplay is on X/Y plane)
        m_pBody->setLinearFactor(btVector3(1, 1, 0));
        // CCD (Continuous Collision Detection): 
        // Prevents fast ball from tunneling through thin blocks/paddle
        m_pBody->setCcdMotionThreshold(radius);
        m_pBody->setCcdSweptSphereRadius(radius * 0.5f);
        // Disable Gravity for the ball (it floats until hit)
        m_pBody->setGravity(btVector3(0, 0, 0));
        // Collides with Paddle, Walls, Blocks (Ignores Powerups, Bullets)
        dynamicsWorld->addRigidBody(m_pBody, COL_BALL, COL_PADDLE | COL_WALL | COL_BLOCK | COL_POWERUP | COL_BALL);
        // Prevent the ball from ever falling asleep (stopping simulation)
        m_pBody->setActivationState(WANTS_DEACTIVATION);
        // Save pointer for collisions
        PhysicsData* pData = new PhysicsData{ TYPE_BALL, this };
        m_pBody->setUserPointer(pData);

        m_maxTrailLength = 10; // Number of ghosts
        m_sampleTimer = 0.0f;

        // Trail defaults
        m_pTrailVB = NULL;
        m_pTrailTexture = NULL;
        m_maxPathSegments = 256; // More segments = smoother curve
        m_ribbonWidth = 0.5f;   // Adjust based on ball size (e.g., m_radius * 2)
        m_minPointDistSq = 0.1f * 0.1f; // Minimum movement to record a point

		InitTrail(device, TEXT(".\\ray_blue.png"));
    }

    ~CArkiBall()
    {
        if (m_pBody && m_pBody->getUserPointer()) delete (PhysicsData*)m_pBody->getUserPointer();
        if (m_pBody && m_pBody->getMotionState()) delete m_pBody->getMotionState();
        if (m_pBody && m_pBody->getCollisionShape()) delete m_pBody->getCollisionShape();
        if (m_pBody)m_pDynamicsWorld->removeRigidBody(m_pBody);
        if (m_pBody) delete m_pBody;
        // Cleanup Graphics
        if (m_pMesh) m_pMesh->Release();
		SAFE_RELEASE(m_pTrailVB);
		SAFE_RELEASE(m_pTrailTexture);
    }
    void SetPosition(float x, float y, float z)
    {   
        btTransform trans = m_pBody->getWorldTransform();
        trans.setOrigin(btVector3(x, y, z));
        m_pBody->setWorldTransform(trans);
        //btTransform trans;
        //m_pBody->getMotionState()->getWorldTransform(trans);
		m_pBody->getMotionState()->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), btVector3(x, y, z)));
    }

    D3DXVECTOR3 GetCameraPos(const D3DXMATRIX& matView)
    {
        D3DXMATRIX matInvView;
        D3DXVECTOR3 cameraPos;

        // The Inverse of the View Matrix is the "Camera's World Matrix"
        // The bottom row of a World Matrix is always the position.
        D3DXMatrixInverse(&matInvView, NULL, &matView);

        cameraPos.x = matInvView._41;
        cameraPos.y = matInvView._42;
        cameraPos.z = matInvView._43;

        return cameraPos;
    }

    // Launch the ball (e.g., when pressing Space)
    void Launch()
    {
		if (m_isActive)return;
        // Activate the body (in case it was sleeping)
        m_pBody->activate(true);
		m_isActive = true;

        // Apply an initial velocity upwards and slightly to the right
        // We normalize the vector and multiply by our target speed
        btVector3 velocity(RandomFloatInRange(-0.5f,0.5f), 1.0f, 0.0f);
        velocity.normalize();
        velocity *= m_targetSpeed;

        m_pBody->setLinearVelocity(velocity);
        m_pBody->setActivationState(DISABLE_DEACTIVATION);

        m_pBody->setRestitution(1.0f);
        m_pBody->setFriction(0.0f);
        m_pBody->setRollingFriction(0.0f);
        m_pBody->setDamping(0.0f, 0.0f);
        m_pBody->setLinearFactor(btVector3(1, 1, 0));
        //m_pBody->setCcdMotionThreshold(radius);
       // m_pBody->setCcdSweptSphereRadius(radius * 0.5f);
        m_pBody->setGravity(btVector3(0, 0, 0));

    }

    // Call this every frame in your Update loop!
    void Update(float dt)
    {
		if (!m_isActive) return;

        if (m_lifetime > 0.0f) {
            m_lifetime -= dt;
            if (m_lifetime <= 0.0f) {
                m_isActive = false;
                // Optionally, you can also set the ball to inactive in physics
                m_pBody->setActivationState(WANTS_DEACTIVATION);
				m_lifetime = 0.0f;
                return;
			}
        }

        // 1. Clamp Speed
        // Physics collisions might slightly reduce speed or add floating point errors.
        // We force the velocity to always be exactly 'm_targetSpeed'.
        btVector3 velocity = m_pBody->getLinearVelocity();
        btScalar currentSpeed = velocity.length();

        // Only normalize if moving (avoid division by zero)
        if (currentSpeed > 0.1f) {
            //velocity.normalize();
            velocity /= currentSpeed; // Normalize
            velocity *= m_targetSpeed; // Scale back to target
            m_pBody->setLinearVelocity(velocity);
        }

        // --- TRAIL LOGIC ---
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        btVector3 currPos = trans.getOrigin();
        D3DXVECTOR3 dxPos((FLOAT)currPos.getX(), (FLOAT)currPos.getY(), (FLOAT)currPos.getZ());

        // Only add a new ghost if we are far enough from the last one
        // (This prevents the trail from bunching up when the ball is slow)
        //if (m_trailHistory.empty() || D3DXVec3LengthSq(&(m_trailHistory.front() - dxPos)) > 0.5f)
        //{
        //    m_trailHistory.push_front(dxPos);

        //    // Remove old ghosts
        //    if (m_trailHistory.size() > m_maxTrailLength)
        //    {
        //        m_trailHistory.pop_back();
        //    }
        //}

        // --- RECORD PATH ---
        btTransform transr;
        m_pBody->getMotionState()->getWorldTransform(transr);
        btVector3 btPosr = transr.getOrigin();
        D3DXVECTOR3 currPosr((FLOAT)btPosr.getX(), (FLOAT)btPosr.getY(), (FLOAT)btPosr.getZ());

        D3DXVECTOR3 d = (m_pathHistory.front() - currPosr);
        // Optimization: Only add point if we moved enough
        if (m_pathHistory.empty() || D3DXVec3LengthSq(&d) > m_minPointDistSq)
        {
            m_pathHistory.push_front(currPosr);

            if (m_pathHistory.size() > m_maxPathSegments)
            {
                m_pathHistory.pop_back();
            }
        }
    }

    void Render(IDirect3DDevice9* device, IDirect3DCubeTexture9* pReflectionTexture, D3DXMATRIX matView, float rotationAngle)
    {
        if (!m_pMesh) return;
        // 1. Get Transform from Physics
        // We ask Bullet where the ball is right now.
        btTransform trans;
        m_pBody->getMotionState()->getWorldTransform(trans);
        // 2. Extract Data Safely
        btVector3 pos = trans.getOrigin();
        btMatrix3x3 rot = trans.getBasis();
        // Convert Bullet Matrix to DirectX Matrix
        D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity(&matWorld);
        // Bullet stores rotation/pos in a specific way, we can extract OpenGL style and convert
        //trans.getOpenGLMatrix((btScalar*)&matWorld);
        // Rotation (Bullet is usually Row-Major in C++, accessing [row][col])
        matWorld._11 = (float)rot[0][0]; matWorld._12 = (float)rot[0][1]; matWorld._13 = (float)rot[0][2];
        matWorld._21 = (float)rot[1][0]; matWorld._22 = (float)rot[1][1]; matWorld._23 = (float)rot[1][2];
        matWorld._31 = (float)rot[2][0]; matWorld._32 = (float)rot[2][1]; matWorld._33 = (float)rot[2][2];

        // Translation (Put in the bottom row for DX9)
        matWorld._41 = (float)pos.getX();
        matWorld._42 = (float)pos.getY();
        matWorld._43 = (float)pos.getZ();
        // Apply to Device
        device->SetTransform(D3DTS_WORLD, &matWorld);

        // 2. Set Material (Shiny White Ball)
        D3DMATERIAL9 mtrl;
        ZeroMemory(&mtrl, sizeof(mtrl));
        mtrl.Diffuse = D3DXCOLOR(0.49f, 0.49f, 0.59f, 1.0f); // White
        mtrl.Ambient = D3DXCOLOR(0.3f, 0.3f, 0.4f, 1.0f); // Grey ambient
        mtrl.Specular = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f); // Bright specular highlight
        mtrl.Power = 40.0f; // Sharp highlight (Plastic/Metal look)

        device->SetMaterial(&mtrl);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
        device->SetRenderState(D3DRS_LIGHTING, TRUE);

        // --- 3. ENABLE REFLECTION MAPPING ---
        if (pReflectionTexture)
        {
            // Bind the Skybox Texture
            device->SetTexture(0, pReflectionTexture);

            // MAGIC: Tell DX9 to automatically calculate the Reflection Vector based on Camera Angle
            device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            // This rotates the 3D reflection vector around the Y axis
            D3DXMATRIX matTextureRot;
            D3DXMatrixRotationX(&matTextureRot, rotationAngle);
            // C. Apply the Matrix to Texture Stage 0
            device->SetTransform(D3DTS_TEXTURE0, &matTextureRot);
            // Tell DX9 this is a Cube Map (requires 3D coordinates)
            device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);

            // Optional: Mix the reflection with the material color (Modulate or Add)
            device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD); // Add makes it look shiny/glowing
           // device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
        }
        // 3. Draw
        m_pMesh->DrawSubset(0);
        // --- 5. CLEANUP (CRITICAL!) ---
        D3DXMATRIX matIdentity;
        D3DXMatrixIdentity(&matIdentity);
        device->SetTransform(D3DTS_TEXTURE0, &matIdentity); // Reset Matrix
        // If you don't turn this off, your Blocks and Text will look crazy/broken
        device->SetTexture(0, NULL);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0); // Reset to standard UVs
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE); // Reset to standard mixing


   //     // 1. ENABLE ALPHA BLENDING (Transparency)
   //     device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   //     device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   //     device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   //     // 2. RENDER GHOSTS
   //     // Iterate through the history
   //     for (size_t i = 0; i < m_trailHistory.size(); i++)
   //     {
   //         D3DXVECTOR3 ghostPos = m_trailHistory[i];

   //         // Math: Calculate fade and shrink factors based on index 'i'
   //         // i=0 is closest to ball (opaque), i=last is furthest (invisible)
   //         float percent = 1.0f - ((float)i / m_maxTrailLength);

   //         float alpha = percent * 0.5f; // Max opacity is 50%
   //         float scale = m_radius * (1.1f + (1.1f * percent)); // Shrink down to 50% size at the tail

   //         // Setup Matrix for Ghost
   //         D3DXMATRIX matWorld, matScale, matTrans;
   //         D3DXMatrixScaling(&matScale, scale, scale, scale); // Use scale instead of full radius
   //         D3DXMatrixTranslation(&matTrans, ghostPos.x, ghostPos.y, ghostPos.z);
   //         matWorld = matScale * matTrans;

   //         device->SetTransform(D3DTS_WORLD, &matWorld);

   //         // Setup Material (Simple color, no reflection for ghosts to save performance)
   //         D3DMATERIAL9 mtrl;
   //         ZeroMemory(&mtrl, sizeof(mtrl));
   //         mtrl.Diffuse = D3DXCOLOR(0.0f, 0.5f, 1.0f, alpha); // Blueish trail
   //         mtrl.Ambient = D3DXCOLOR(0.0f, 0.5f, 1.0f, alpha);
			//mtrl.Emissive = D3DXCOLOR(0.5f, 0.5f, 0.9f, alpha); // No specular
   //         device->SetMaterial(&mtrl);

   //         // Disable Texture/Reflection for trail (optional, looks cleaner)
   //         //device->SetTexture(0, NULL);
   //         //device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
   //        // device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);

   //         m_pMesh->DrawSubset(0);
   //     }

   //     // 3. RENDER REAL BALL (Standard Code)
   //     // ... [Insert your existing Real Ball Render Code here] ...

        // Remember to restore the Render States at the very end!
       // 
        D3DXVECTOR3 camPos = GetCameraPos(matView);
        //device->SetMaterial(NULL);

		RenderTrail(device, camPos); // Render the ribbon trail

    }

    void InitTrail(IDirect3DDevice9* device, const TCHAR* textureFilename);
    void RenderTrail(IDirect3DDevice9* device, const D3DXVECTOR3& cameraPos); // New render function
    void OnDeviceLost();
    void OnDeviceReset(IDirect3DDevice9* device);
};