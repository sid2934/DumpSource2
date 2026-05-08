class CBasePropDoor : public CDynamicProp
{
	float32 m_flAutoReturnDelay;
	// MNotSaved
	CUtlVector< CHandle< CBasePropDoor > > m_hDoorList;
	int32 m_nHardwareType;
	bool m_bNeedsHardware;
	DoorState_t m_eDoorState;
	bool m_bLocked;
	bool m_bNoNPCs;
	Vector m_closedPosition;
	QAngle m_closedAngles;
	CHandle< CBaseEntity > m_hBlocker;
	bool m_bFirstBlocked;
	locksound_t m_ls;
	bool m_bForceClosed;
	VectorWS m_vecLatchWorldPosition;
	CHandle< CBaseEntity > m_hActivator;
	CUtlSymbolLarge m_SoundMoving;
	CUtlSymbolLarge m_SoundOpen;
	CUtlSymbolLarge m_SoundClose;
	CUtlSymbolLarge m_SoundLock;
	CUtlSymbolLarge m_SoundUnlock;
	CUtlSymbolLarge m_SoundLatch;
	// MNotSaved
	CUtlSymbolLarge m_SoundPound;
	CUtlSymbolLarge m_SoundJiggle;
	CUtlSymbolLarge m_SoundLockedAnim;
	// MNotSaved
	int32 m_numCloseAttempts;
	// MNotSaved
	CUtlStringToken m_nPhysicsMaterial;
	CUtlSymbolLarge m_SlaveName;
	CHandle< CBasePropDoor > m_hMaster;
	CEntityIOOutput m_OnBlockedClosing;
	CEntityIOOutput m_OnBlockedOpening;
	CEntityIOOutput m_OnUnblockedClosing;
	CEntityIOOutput m_OnUnblockedOpening;
	CEntityIOOutput m_OnFullyClosed;
	CEntityIOOutput m_OnFullyOpen;
	CEntityIOOutput m_OnClose;
	CEntityIOOutput m_OnOpen;
	CEntityIOOutput m_OnLockedUse;
	CEntityIOOutput m_OnAjarOpen;
};
