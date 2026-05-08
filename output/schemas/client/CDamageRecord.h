class CDamageRecord
{
	CHandle< C_CSPlayerPawn > m_PlayerDamager;
	CHandle< C_CSPlayerPawn > m_PlayerRecipient;
	CHandle< CCSPlayerController > m_hPlayerControllerDamager;
	CHandle< CCSPlayerController > m_hPlayerControllerRecipient;
	CUtlString m_szPlayerDamagerName;
	CUtlString m_szPlayerRecipientName;
	uint64 m_DamagerXuid;
	uint64 m_RecipientXuid;
	float32 m_flBulletsDamage;
	float32 m_flDamage;
	float32 m_flActualHealthRemoved;
	int32 m_iNumHits;
	int32 m_iLastBulletUpdate;
	bool m_bIsOtherEnemy;
	EKillTypes_t m_killType;
};
