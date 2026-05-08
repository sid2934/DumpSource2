class CMolotovProjectile : public CBaseCSGrenadeProjectile
{
	bool m_bIsIncGrenade;
	bool m_bDetonated;
	IntervalTimer m_stillTimer;
};
