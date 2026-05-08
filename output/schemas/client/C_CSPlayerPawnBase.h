class C_CSPlayerPawnBase : public C_BasePlayerPawn
{
	CCSPlayer_PingServices* m_pPingServices;
	CSPlayerState m_previousPlayerState;
	CSPlayerState m_iPlayerState;
	bool m_bHasMovedSinceSpawn;
	GameTime_t m_flLastSpawnTimeIndex;
	int32 m_iProgressBarDuration;
	float32 m_flProgressBarStartTime;
	GameTime_t m_flClientDeathTime;
	float32 m_flFlashBangTime;
	float32 m_flFlashScreenshotAlpha;
	float32 m_flFlashOverlayAlpha;
	bool m_bFlashBuildUp;
	bool m_bFlashDspHasBeenCleared;
	bool m_bFlashScreenshotHasBeenGrabbed;
	float32 m_flFlashMaxAlpha;
	float32 m_flFlashDuration;
	GameTime_t m_flClientHealthFadeChangeTimestamp;
	int32 m_nClientHealthFadeParityValue;
	float32 m_fNextThinkPushAway;
	float32 m_flCurrentMusicStartTime;
	float32 m_flMusicRoundStartTime;
	bool m_bDeferStartMusicOnWarmup;
	float32 m_flLastSmokeOverlayAlpha;
	float32 m_flLastSmokeAge;
	Vector m_vLastSmokeOverlayColor;
	CHandle< CCSPlayerController > m_hOriginalController;
};
