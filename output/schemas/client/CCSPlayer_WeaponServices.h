class CCSPlayer_WeaponServices : public CPlayer_WeaponServices
{
	GameTime_t m_flNextAttack;
	uint32 m_nOldTotalShootPositionHistoryCount;
	uint32 m_nOldTotalInputHistoryCount;
	C_NetworkUtlVectorBase< uint8 > m_networkAnimTiming;
	bool m_bBlockInspectUntilNextGraphUpdate;
};
