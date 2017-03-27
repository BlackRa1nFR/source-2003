/**********************************************************************
	FILE: Qhelpers.h

	DESCRIPTION: Quake Helper object header file

	CREATED BY: Phil Camp

	HISTORY: Based on helpers.h 10 Oct 1996

 	Copyright (c) 1996, Phil Camp, A.P. Designs Ltd. All Rights Reserved.

 **********************************************************************/

#ifndef __QHELPERS__H
#define __QHELPERS__H


TCHAR *GetString(int id);

// Class IDs

// This is the main Class ID for the entity helper
#define QENTHELP_CLASS_ID   Class_ID(0x41d53afe, 0x616c0f57)

// The following class IDs are for the 'shortcut' versions of
// specific entities.
#define QDOOR_CLASS_ID      Class_ID(0x3ebf593d, 0x1db61362)
#define QSDOOR_CLASS_ID     Class_ID(0x597d2f7e, 0x2aa5bca)
#define QPLATFORM_CLASS_ID  Class_ID(0x7264153d, 0x4eb653f4)
#define QMPLAT_CLASS_ID     Class_ID(0x4e3d33c6, 0x498f59f5)
#define QBUTTON_CLASS_ID    Class_ID(0x697908ee, 0x2b714e73)
#define QTRIG1_CLASS_ID     Class_ID(0x68f938e5, 0x48f30344)
#define QTRIGM_CLASS_ID     Class_ID(0x325027e5, 0x258f50c5)
#define QTRIGS_CLASS_ID     Class_ID(0x73bb7708, 0x76ae3788)
#define QTRIGC_CLASS_ID     Class_ID(0x10be39f4, 0x4cdb51be)
#define QTELET_CLASS_ID     Class_ID(0x2100089d, 0x5176258c)
#define QTELED_CLASS_ID     Class_ID(0x638e21a9, 0x6b1615c4)

// class descriptors
extern ClassDesc* GetQEntHelpDesc();
extern ClassDesc* GetQDoorHelpDesc();
extern ClassDesc* GetQSDoorHelpDesc();
extern ClassDesc* GetQPlatformHelpDesc();
extern ClassDesc* GetQMPlatHelpDesc();
extern ClassDesc* GetQButtonHelpDesc();
extern ClassDesc* GetQTrig1HelpDesc();
extern ClassDesc* GetQTrigMHelpDesc();
extern ClassDesc* GetQTrigSHelpDesc();
extern ClassDesc* GetQTrigCHelpDesc();
extern ClassDesc* GetQTeleTHelpDesc();
extern ClassDesc* GetQTeleDHelpDesc();

// this class conatins the Quake specific data for an Entity
class QEntMapData
{
public:
  int   Angle ;
  int   Delay ;
  int   Dmg ;
  BOOL  DoorDontLink ;
  BOOL  FirstDown ;
  BOOL  FirstLeft ;
  BOOL  GoldKey ;
  int   Health ;
  int   Height ;
  char  KillTarget [32] ;
  BOOL  Laser ;
  int   Light ;
  int   Lip ;
  Point3 Mangle ;
  char  Map [80] ;
  char  Message [80] ;
  int   NextThink ;
  BOOL  NoIntermisson ;
  BOOL  NoMessage ;
  BOOL  NoTouch ;
  BOOL  OpenOnce ;
  BOOL  PlayerOnly ;
  BOOL  PushOnce ;
  BOOL  Silent ;
  BOOL  SilverKey ;
  int   Sounds ;
  int   SpawnFlags ;
  int   Speed ;
  int   Style ;
  BOOL  StartOpen ;
  BOOL  StartsOff ;
  BOOL  SuperSpike ;
  char  Target [32] ;
  char  TargetName [32] ;
  BOOL  Toggle ;
  int   Wait ;
#define MAX_MAP_PARAMS 128
  char  MapParams [MAX_MAP_PARAMS] ;
} ;


#define QT_GENERAL      0
#define QT_DOOR         1

// the main entity helper class
class QEntHelpObject: public HelperObject 
{
	public:			
		// Class vars
		static HWND hParams;
    static HWND hClassParams ;
    static HWND hMapParams ;
    static int  ClassParamsID ;
		static IObjParam *iObjParams;
		static int dlgShowAxis;
		static float dlgAxisLength;

    QEntMapData dlgData ; 
 
    // Quake data
    char QClassName [80] ;
    int  QEntityType ;

		// Snap suspension flag (TRUE during creation only)
		BOOL suspendSnap;
					
		// Params
		BOOL showAxis;
		float axisLength;
 		int extDispFlags;

    // entity specific Quake data
    QEntMapData Data ;

		//  inherited virtual methods for Reference-management
		RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message );		
	
		QEntHelpObject(char* QName );
		~QEntHelpObject();
		
		// From BaseObject
		int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		void SetExtendedDisplay(int flags);
		int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		CreateMouseCallBack* GetCreateMouseCallBack();
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

		void BeginEditClassParams( IObjParam *ip, ULONG flags);
		void EndEditClassParams( IObjParam *ip, ULONG flags);

    void BeginEditMapParams ( IObjParam *ip, ULONG flags);
		void EndEditMapParams ( IObjParam *ip, ULONG flags);

		// From Object
		ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) ;
		ObjectHandle ApplyTransform(Matrix3& matrix) {return this;}
		Interval ObjectValidity(TimeValue t) {return FOREVER;}
		int CanConvertToType(Class_ID obtype) {return FALSE;}
		Object* ConvertToType(TimeValue t, Class_ID obtype) {assert(0);return NULL;}		
		void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
		void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
		int DoOwnSelectHilite()	{ return 1; }

		// Animatable methods
		void DeleteThis() { delete this; }
		Class_ID ClassID() { return QENTHELP_CLASS_ID; }  
		void GetClassName(TSTR& s) ;
		int IsKeyable(){ return 0;}
		
		// From ref
		RefTargetHandle Clone(RemapDir& remap = NoRemap());
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

    // quake methods
    void SetTypeFromName() ;
    void EncodeMapParams() ;
    void DecodeMapParams( HWND hMapDlg) ;
	};				


#endif // __QHELPERS__H
