class CCSPlayer_AimPunchServices : public CPlayerPawnComponent
{
	GameTick_t m_predictableBaseTick;
	float32 m_predictableBaseTickInterpAmount;
	QAngle m_predictableBaseAngle;
	QAngle m_predictableBaseAngleVel;
	GameTick_t m_unpredictableBaseTick;
	QAngle m_unpredictableBaseAngle;
};
