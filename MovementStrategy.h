#pragma once

// Forward Declaration
class CFlyingEnemy;

// --------------------------------------------------------
// STRATEGY PATTERN INTERFACE
// --------------------------------------------------------
class IMovementStrategy
{
public:
    virtual ~IMovementStrategy() {}

    // Returns TRUE if the movement is "Complete" (e.g. reached bottom)
    virtual bool Update(CFlyingEnemy* enemy, double dt) = 0;
    // Called when we switch to this state (reset timers, etc.)
    virtual void OnEnter(CFlyingEnemy* enemy) = 0;
};

// --------------------------------------------------------
// STRATEGY 1: SINE WAVE ATTACK (Moves Down + Wobble)
// --------------------------------------------------------
class CSineMove : public IMovementStrategy
{
    float m_totalTime;
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};
// --------------------------------------------------------
// STRATEGY: STEERING SINE WAVE (Drifting Organic Wave)
// --------------------------------------------------------
class CSineMove2 : public IMovementStrategy
{
    float m_totalTime;
    D3DXVECTOR3 m_velocity;
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};
// --------------------------------------------------------
// STRATEGY 2: RAPID RETURN (Moves Up + Centers X)
// --------------------------------------------------------
class CReturnMove : public IMovementStrategy
{
    float m_t; // Progress from 0.0 to 1.0
    D3DXVECTOR3 m_p0; // Start
    D3DXVECTOR3 m_p1; // Control Point (The "Curve Magnet")
    D3DXVECTOR3 m_p2; // End (Home)
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};
// --------------------------------------------------------
// STRATEGY 2.1: STEERING RETURN (Arrival Behavior)
// --------------------------------------------------------
class CReturnMove2 : public IMovementStrategy
{
    D3DXVECTOR3 m_velocity; // We track our own physics
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};
// --------------------------------------------------------
// STRATEGY 3: ZIG-ZAG (Ping Pong Movement)
// --------------------------------------------------------
class CZigZagMove : public IMovementStrategy
{
    float m_direction; // +1 or -1
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};

// --------------------------------------------------------
// STRATEGY 4: SPIRAL DIVE (Loop-de-loops downwards)
// --------------------------------------------------------
class CSpiralMove : public IMovementStrategy
{
    float m_angle;
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};

// --------------------------------------------------------
// STRATEGY 5: FIGURE EIGHT (The "Butterfly" Pattern)
// --------------------------------------------------------
class CFigureEightMove : public IMovementStrategy
{
    float m_totalTime;
public:
    virtual void OnEnter(CFlyingEnemy* enemy);
    virtual bool Update(CFlyingEnemy* enemy, double dt);
};