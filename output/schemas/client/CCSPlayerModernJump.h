class CCSPlayerModernJump
{
	GameTick_t m_nLastActualJumpPressTick;
	float32 m_flLastActualJumpPressFrac;
	GameTick_t m_nLastUsableJumpPressTick;
	float32 m_flLastUsableJumpPressFrac;
	GameTick_t m_nLastLandedTick;
	float32 m_flLastLandedFrac;
	float32 m_flLastLandedVelocityX;
	float32 m_flLastLandedVelocityY;
	float32 m_flLastLandedVelocityZ;
};
