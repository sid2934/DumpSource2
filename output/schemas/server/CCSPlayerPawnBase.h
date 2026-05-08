class CCSPlayerPawnBase : public CBasePlayerPawn
{
	CTouchExpansionComponent m_CTouchExpansionComponent;
	CCSPlayer_PingServices* m_pPingServices;
	GameTime_t m_blindUntilTime;
	GameTime_t m_blindStartTime;
	CSPlayerState m_iPlayerState;
	bool m_bRespawning;
	bool m_bHasMovedSinceSpawn;
	int32 m_iNumSpawns;
	float32 m_flIdleTimeSinceLastAction;
	float32 m_fNextRadarUpdateTime;
	float32 m_flFlashDuration;
	float32 m_flFlashMaxAlpha;
	float32 m_flProgressBarStartTime;
	int32 m_iProgressBarDuration;
	CHandle< CCSPlayerController > m_hOriginalController;
};
