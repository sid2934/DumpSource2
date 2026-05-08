class CLogicEventListener : public CLogicalEntity
{
	CUtlString m_strEventName;
	bool m_bIsEnabled;
	int32 m_nTeam;
	CEntityOutputTemplate< CUtlString > m_OnEventFired;
};
