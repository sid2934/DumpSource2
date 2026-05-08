// MGetKV3ClassDefaults = {
//	"m_sceneInstance": null,
//	"m_mainActor": null,
//	"m_cursorIDToEventID":
//	{
//	}
//}
class CPulseCell_Outflow_PlaySceneBase::CursorState_t
{
	CHandle< CBaseEntity > m_sceneInstance;
	CHandle< CBaseEntity > m_mainActor;
	CUtlHashtable< PulseCursorID_t, int32 > m_cursorIDToEventID;
};
