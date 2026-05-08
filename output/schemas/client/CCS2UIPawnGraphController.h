// MGetKV3ClassDefaults = {
//	"_class": "CCS2UIPawnGraphController",
//	"m_hExternalGraph": 4294967295,
//	"m_nAnimationSeed": null,
//	"m_characterMode": null,
//	"m_nTeamPreviewVariant": null,
//	"m_nTeamPreviewRandom": null,
//	"m_nTeamPreviewPosition": null,
//	"m_endOfMatchCelebration": null,
//	"m_action": null,
//	"m_bannerAnimation": null,
//	"m_weaponCategory": null,
//	"m_weaponType": null,
//	"m_weaponState": null,
//	"m_inspectTurnAngle": null,
//	"m_bCT": null
//}
class CCS2UIPawnGraphController : public CAnimGraphControllerBase
{
	CAnimGraph2ParamOptionalRef< float32 > m_nAnimationSeed;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_characterMode;
	CAnimGraph2ParamOptionalRef< float32 > m_nTeamPreviewVariant;
	CAnimGraph2ParamOptionalRef< float32 > m_nTeamPreviewRandom;
	CAnimGraph2ParamOptionalRef< float32 > m_nTeamPreviewPosition;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_endOfMatchCelebration;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_action;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_bannerAnimation;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_weaponCategory;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_weaponType;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_weaponState;
	CAnimGraph2ParamOptionalRef< float32 > m_inspectTurnAngle;
	CAnimGraph2ParamOptionalRef< bool > m_bCT;
};
