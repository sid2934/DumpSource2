class CCSPlayerController_ActionTrackingServices : public CPlayerControllerComponent
{
	CUtlVectorEmbeddedNetworkVar< CSPerRoundStats_t > m_perRoundStats;
	CSMatchStats_t m_matchStats;
	int32 m_iNumRoundKills;
	int32 m_iNumRoundKillsHeadshots;
	float32 m_flTotalRoundDamageDealt;
};
