class CBaseAnimGraph : public CBaseModelEntity
{
	// MSaveOpsForField = "GetAnimGraphControllerManagerSaveRestoreOps"
	CAnimGraphControllerManager m_graphControllerManager;
	// MSaveOpsForField = "GetAnimGraphControllerPtrSaveRestoreOps"
	CAnimGraphControllerBase* m_pMainGraphController;
	bool m_bInitiallyPopulateInterpHistory;
	// MSaveOpsForField = "GetChoreoServicesSaveRestoreOps"
	IChoreoServices* m_pChoreoServices;
	bool m_bAnimGraphUpdateEnabled;
	// MNotSaved
	bool m_bAnimationUpdateScheduled;
	// MNotSaved
	Vector m_vecForce;
	// MNotSaved
	int32 m_nForceBone;
	// MPhysPtr
	IPhysicsRagdollControl* m_pRagdollControl;
	PhysicsRagdollPose_t m_RagdollPose;
	bool m_bRagdollEnabled;
	// MNotSaved
	bool m_bRagdollClientSide;
	CTransform m_xParentedRagdollRootInEntitySpace;
};
