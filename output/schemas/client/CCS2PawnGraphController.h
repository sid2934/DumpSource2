// MGetKV3ClassDefaults = {
//	"_class": "CCS2PawnGraphController",
//	"m_hExternalGraph": 4294967295,
//	"m_action": null,
//	"m_bActionReset": null,
//	"m_flWeaponActionSpeedScale": null,
//	"m_weaponCategory": null,
//	"m_weaponType": null,
//	"m_weaponExtraInfo": null,
//	"m_flWeaponAmmo": null,
//	"m_flWeaponAmmoMax": null,
//	"m_flWeaponAmmoReserve": null,
//	"m_bWeaponIsSilenced": null,
//	"m_flWeaponIronsightAmount": null,
//	"m_bIsUsingLegacyModel": null,
//	"m_idleVariation": null,
//	"m_deployVariation": null,
//	"m_attackType": null,
//	"m_attackThrowStrength": null,
//	"m_flAttackVariation": null,
//	"m_inspectVariation": null,
//	"m_inspectExtraInfo": null,
//	"m_reloadStage": null,
//	"m_bIsDefusing": null,
//	"m_moveType": null,
//	"m_moveDirectionID": null,
//	"m_flMoveSpeedX": null,
//	"m_flMoveSpeedY": null,
//	"m_flMoveSpeedHorizontal": null,
//	"m_flPreviousMoveSpeedHorizontal": null,
//	"m_flCrouchAmount": null,
//	"m_bIsWalking": null,
//	"m_flWeaponDropAmount": null,
//	"m_groundAction": null,
//	"m_groundActionDirectionID": null,
//	"m_flGroundTurnAngleOrVelocity": null,
//	"m_flLadderCycle": null,
//	"m_flLadderYaw": null,
//	"m_flLadderYawBackwards": null,
//	"m_airAction": null,
//	"m_flAirHeightAboveGround": null,
//	"m_leftFootTarget": null,
//	"m_rightFootTarget": null,
//	"m_flFlashedAmount": null,
//	"m_flAimPitchAngle": null,
//	"m_flAimYawAngle": null,
//	"m_flinchHead": null,
//	"m_flinchHeadRestart": null,
//	"m_flinchBody": null,
//	"m_flinchBodyRestart": null,
//	"m_flinchIsOnFire": null
//}
class CCS2PawnGraphController : public CCS2WeaponGraphController
{
	CAnimGraph2ParamOptionalRef< bool > m_bIsDefusing;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_moveType;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_moveDirectionID;
	CAnimGraph2ParamOptionalRef< float32 > m_flMoveSpeedX;
	CAnimGraph2ParamOptionalRef< float32 > m_flMoveSpeedY;
	CAnimGraph2ParamOptionalRef< float32 > m_flMoveSpeedHorizontal;
	CAnimGraph2ParamOptionalRef< float32 > m_flPreviousMoveSpeedHorizontal;
	CAnimGraph2ParamOptionalRef< float32 > m_flCrouchAmount;
	CAnimGraph2ParamOptionalRef< bool > m_bIsWalking;
	CAnimGraph2ParamOptionalRef< float32 > m_flWeaponDropAmount;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_groundAction;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_groundActionDirectionID;
	CAnimGraph2ParamOptionalRef< float32 > m_flGroundTurnAngleOrVelocity;
	CAnimGraph2ParamOptionalRef< float32 > m_flLadderCycle;
	CAnimGraph2ParamOptionalRef< float32 > m_flLadderYaw;
	CAnimGraph2ParamOptionalRef< float32 > m_flLadderYawBackwards;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_airAction;
	CAnimGraph2ParamOptionalRef< float32 > m_flAirHeightAboveGround;
	CAnimGraph2ParamOptionalRef< CNmTarget > m_leftFootTarget;
	CAnimGraph2ParamOptionalRef< CNmTarget > m_rightFootTarget;
	CAnimGraph2ParamOptionalRef< float32 > m_flFlashedAmount;
	CAnimGraph2ParamOptionalRef< float32 > m_flAimPitchAngle;
	CAnimGraph2ParamOptionalRef< float32 > m_flAimYawAngle;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_flinchHead;
	CAnimGraph2ParamOptionalRef< bool > m_flinchHeadRestart;
	CAnimGraph2ParamOptionalRef< CGlobalSymbol > m_flinchBody;
	CAnimGraph2ParamOptionalRef< bool > m_flinchBodyRestart;
	CAnimGraph2ParamOptionalRef< bool > m_flinchIsOnFire;
};
