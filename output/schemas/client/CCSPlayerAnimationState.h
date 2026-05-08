class CCSPlayerAnimationState
{
	CCSPlayerAnimationState::MoveType_t m_currentMoveType;
	CCSPlayerAnimationState::GroundMoveState_t m_groundMoveState;
	CCSPlayerAnimationState::Direction_t m_groundActionDirection;
	CCSPlayerAnimationState::AirAction_t m_airAction;
	bool m_bWasOnGroundLastUpdate;
	bool m_bWasStationaryLastUpdate;
	GameTick_t m_actionStartTick;
	GameTick_t m_staticAimTimerStartTick;
	GameTick_t m_plantAndTurnStartTick;
	float32 m_flTurnOnSpotAngle;
	float32 m_flPreviousAimYaw;
	float32 m_flPreviousHorizontalSpeed;
	float32 m_flFootIKOffsetLeft;
	float32 m_flFootIKOffsetRight;
	float32 m_flWeaponDropPercentageDueToMovement;
	float32 m_flWeaponDropSmoothDampVelocity;
};
