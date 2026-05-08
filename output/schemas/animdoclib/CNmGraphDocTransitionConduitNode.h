// MGetKV3ClassDefaults = {
//	"_class": "CNmGraphDocTransitionConduitNode",
//	"m_ID": <HIDDEN FOR DIFF>,
//	"m_name": "",
//	"m_floatingComment": "",
//	"m_position":
//	[
//		0.000000,
//		0.000000
//	],
//	"m_pChildGraph": null,
//	"m_pSecondaryGraph":
//	{
//		"_class": "CNmGraphDocFlowGraph",
//		"m_ID": <HIDDEN FOR DIFF>,
//		"m_nodes":
//		[
//		],
//		"m_graphType": "TransitionConduit",
//		"m_viewOffset":
//		[
//			0.000000,
//			0.000000
//		],
//		"m_flViewZoom": 1.000000,
//		"m_connections":
//		[
//		]
//	},
//	"m_startStateID": "00010000-0000-0000-cccc-cccccccccccc",
//	"m_endStateID": "0000ffff-0000-0000-3cb2-68b1093a0000"
//}
class CNmGraphDocTransitionConduitNode : public CNmGraphDocStateMachineGraphNode
{
	V_uuid_t m_startStateID;
	V_uuid_t m_endStateID;
};
