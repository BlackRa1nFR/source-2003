/**********************************************************************

  FILE: qmapexp.cpp

	DESCRIPTION:  Quake Map file export module

	CREATED BY: Phil Camp (aka 'flametop')

	HISTORY: created 30 September 1996

	Copyright (c) 1996 Phil Camp, A.P. Designs Ltd. All Rights Reserved.

  Based on MAX SDK Example (c) 1996 Kintetix Inc.

 **********************************************************************/

#include "..\include\Max.h"
#include "..\include\StdMat.h"
#include "dxferes.h"

// get definitions of the Quake Helpers from QENTHLP plugin
#include "..\QEntHlp\qhelpers.h"

HINSTANCE hInstance;

// *********************************************************************
// General routines

TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
} ;

static void MessageBox(int s1, int s2) 
{
	TSTR str1(GetString(s1));
	TSTR str2(GetString(s2));
	MessageBox(GetActiveWindow(), str1.data(), str2.data(), MB_OK);
} ;

static int MessageBox(int s1, int s2, int option = MB_OK) 
{
	TSTR str1(GetString(s1));
	TSTR str2(GetString(s2));
	return MessageBox(GetActiveWindow(), str1, str2, option);
} ;

static int Alert(int s1, int s2 = IDS_QMAPEXP, int option = MB_OK) 
{
	return MessageBox(s1, s2, option);
} ;


#define no_RAM() Alert(IDS_TH_OUTOFMEMORY)

// *********************************************************************
// Export class

#define NUM_SOURCES 3 		// # of sources in dialog

class QMapExport : public SceneExport 
{
	friend BOOL CALLBACK ExportOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

public:
					      QMapExport();
					      ~QMapExport();
	int				    ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage(); // ASCII Copyright message
	const TCHAR *	OtherMessage1();		// Other message #1
	const TCHAR *	OtherMessage2();		// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	int	DoExport (const TCHAR *name,ExpInterface *ei,Interface *i);	// Export file

  // attributes from dialog
  char MapName [80] ;     // name Quake displays on entry to level
  char WADName [80] ;     // name of WAD file for brush textures
  int WorldType ;         // Quake level type
};

// ******************************************************************************
// Handy file class

class WorkFile
{
private:
	FILE *stream;
	
public:
  WorkFile(const TCHAR *filename,const TCHAR *mode) { stream = NULL; Open(filename, mode); };
  ~WorkFile() { Close(); };

  FILE *  Stream() { return stream; };
  int     Close() { int result=0; if(stream) result=fclose(stream); stream = NULL; return result; }
	void    Open(const TCHAR *filename,const TCHAR *mode) { Close(); stream = _tfopen(filename,mode); }
};

// ******************************************************************************
// Handy memory worker

class Memory
{
	void *ptr;
public:
  Memory() { ptr = NULL; }
	Memory(int amount, BOOL zero = FALSE) { ptr = NULL; Alloc(amount, zero); }
	~Memory() { Free(); }
	void *			Ptr() { return ptr; }
	void *			Realloc(int amount);
	void *			Alloc(int amount, BOOL zero = FALSE);
	void			  Free() { if(ptr) free(ptr); ptr = NULL; }
};

void *Memory::Realloc(int amount)
{
	if(ptr)
		ptr = realloc(ptr, amount);
	else
		ptr = malloc(amount);
	return ptr;
} ;

void *Memory::Alloc(int amount, BOOL zero) 
{
	Free();
	ptr = malloc(amount);
	if(ptr && zero) {
		char *p = (char *)ptr;
		for(int i = 0; i < amount; ++i)
			*p++ = 0;
		}
	return ptr;
} ;

// ******************************************************************************
// Jaguar interface code

int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	hInstance = hinstDLL;

	if ( !controlsInit )
  {
		controlsInit = TRUE;
		
		// jaguar controls
		InitCustomControls(hInstance);

		// initialize Chicago controls
		InitCommonControls();
	} ;
	switch(fdwReason) 
  {
		case DLL_PROCESS_ATTACH:
			//MessageBox(NULL,_T("DXFIMP.DLL: DllMain"),_T("DXFIMP"),MB_OK);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	} ;
	return(TRUE);
} ;


// ******************************************************************************

class QMapClassDesc:public ClassDesc 
{
	public:
	int         IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new QMapExport; }
	const TCHAR *	ClassName() { return GetString(IDS_QMAPEXP); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return Class_ID(0x4b7f076f, 0x6af56493); }
	const TCHAR* 	Category() { return GetString(IDS_TH_SCENEEXPORT);  }
};

static QMapClassDesc QMapDesc;

//------------------------------------------------------
// This is the interface to Jaguar:
//------------------------------------------------------

__declspec( dllexport ) const TCHAR *
LibDescription() 
{ 
  return GetString(IDS_TH_QMAPEXPORTDLL); 
} ;

__declspec( dllexport ) int
LibNumberClasses() 
{ 
  return 1; 
} ;

__declspec( dllexport ) ClassDesc *
LibClassDesc(int i) 
{
	switch(i)
  {
		case 0: return &QMapDesc; break;
		default: return 0; break;
	}
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG 
LibVersion() 
{ 
  return 100;
}

// ******************************************************************************
//
// QMap export module functions follow:
//

QMapExport::QMapExport()
{
} ;

QMapExport::~QMapExport() 
{
} ;

int QMapExport::ExtCount() 
{
	return 1;       // we support only 1 file ext (MAP)
} ;

const TCHAR * QMapExport::Ext(int n) 
{		// Extensions supported for import/export modules
	switch(n) 
  {
		case 0:
			return _T("MAP");
	}
	return _T("");
} ;

const TCHAR * QMapExport::LongDesc() 
{
  // Long ASCII description 
	return GetString(IDS_TH_QUAKEMAPFILE);
} ;
	
const TCHAR * QMapExport::ShortDesc()
{
  // short ASCII description 
	return GetString(IDS_TH_QUAKE);
}

const TCHAR * QMapExport::AuthorName()
{
  // ASCII Author name
	return GetString(IDS_TH_PHIL_CAMP);
} ;

const TCHAR * QMapExport::CopyrightMessage() 
{
  // ASCII Copyright message
	return GetString(IDS_TH_COPYRIGHT_BW);
} ;

const TCHAR * QMapExport::OtherMessage1()
{
	// Other message #1
	return _T("phil@flametop.demon.co.uk");
} ;

const TCHAR * QMapExport::OtherMessage2() 
{
	// Other message #2
	return _T("");
} ;

unsigned int QMapExport::Version() 
{				
  // Version number * 100 (i.e. v3.01 = 301)
	return 104;
}

void QMapExport::ShowAbout(HWND hWnd) 
{	
} ;


static BOOL CALLBACK ExportOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
	static QMapExport *exp;

	switch(message) 
  {
		case WM_INITDIALOG:
			exp = (QMapExport *)lParam;
			CenterWindow(hDlg,GetParent(hDlg));
			SetFocus(hDlg); // For some reason this was necessary.  DS-3/4/96
			return FALSE;
		case WM_DESTROY:
			return FALSE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
      {
				case IDOK: 
        {
					// Unload Quake dialog values into map data
          GetDlgItemText(hDlg,IDC_MAPNAME,(LPSTR)exp->MapName,sizeof(exp->MapName)) ;
          GetDlgItemText(hDlg,IDC_WADNAME,(LPSTR)exp->WADName,sizeof(exp->WADName)) ;
          exp->WorldType = 0 ;
          if (IsDlgButtonChecked(hDlg,IDC_WT_MEDIEVAL))
            exp->WorldType = 0 ;
          if (IsDlgButtonChecked(hDlg,IDC_WT_RUNIC))
            exp->WorldType = 1 ;
          if (IsDlgButtonChecked(hDlg,IDC_WT_PRESENT))
            exp->WorldType = 2 ;

          // if no WAD name, default to medieval.wad
          if (strlen(exp->WADName) == 0)
            strcpy(exp->WADName,"medieval.wad") ;

					EndDialog(hDlg, 1);
        }
				return TRUE;
				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;
			} ;
	} ;
	return FALSE;
}

// *************************************************************************
// MyScene classes

// node object types we process
#define OBTYPE_MESH       0
#define OBTYPE_OMNILIGHT  1
#define OBTYPE_QENTITY    2

class MySceneEntry 
{
	public:
		INode *node;
		Object *obj;
    int type ;    // see above
		MySceneEntry *next;
		MySceneEntry(INode *n, Object *o, int t) { node = n; obj = o; type = t ; next = NULL; }
};

class MySceneEnumProc : public ITreeEnumProc 
{
	public:
		Interface	*i;
		MySceneEntry *head;
		MySceneEntry *tail;
		IScene		*theScene;
		int			count;
		TimeValue	time;

    MySceneEnumProc(IScene *scene, TimeValue t, Interface *i);
		~MySceneEnumProc();
		int			Count() { return count; }
		void		Append(INode *node, Object *obj, int type);
		int			callback( INode *node );
		Box3		Bound();
		MySceneEntry *operator[](int index);
};

MySceneEnumProc::MySceneEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	time = t;
	theScene = scene;
	count = 0;
	head = tail = NULL;
	this->i = i;
	theScene->EnumTree(this);
}

MySceneEnumProc::~MySceneEnumProc() 
{
	while(head)
  {
		MySceneEntry *next = head->next;
		delete head;
		head = next;
	}
	head = tail = NULL;
	count = 0;	
}

int MySceneEnumProc::callback(INode *node) 
{
  // get object from node
	Object *obj = node->EvalWorldState(time).obj;
  // check if it can be represented by a mesh
	if(obj->CanConvertToType(triObjectClassID)) 
  {
    // create a MySceneEntry for the meshable object
		Append(node, obj, OBTYPE_MESH);
	}
  else
  {
    // check for other valid types
  	switch (obj->SuperClassID())
    { 
		  case HELPER_CLASS_ID:
        // check for entity helper class ID
			  if ( obj->ClassID()==QENTHELP_CLASS_ID) 
				  Append(node, obj, OBTYPE_QENTITY);
			  break;
		  case LIGHT_CLASS_ID:
        // check for omni light
			  if (obj->ClassID()==Class_ID(OMNI_LIGHT_CLASS_ID,0))
				  Append(node, obj, OBTYPE_OMNILIGHT);
			  break;
		}
  } ;
	return TREE_CONTINUE;	// Keep on enumeratin'!
}

void MySceneEnumProc::Append(INode *node, Object *obj, int type) 
// add node/object/type to MySceneEntry list
{
	MySceneEntry *entry = new MySceneEntry(node, obj, type);
	if(tail)
		tail->next = entry;
	tail = entry;
	if(!head)
		head = entry;
	count++;	
}

Box3 MySceneEnumProc::Bound() 
{
	Box3 bound;
	bound.Init();
	MySceneEntry *e = head;
	ViewExp *vpt = i->GetViewport(NULL);
	while(e)
  {
		Box3 bb;
		e->obj->GetWorldBoundBox(time, e->node, vpt, bb);
		bound += bb;
		e = e->next;
	} ;
	return bound;
}

MySceneEntry *MySceneEnumProc::operator[](int index) 
{
	MySceneEntry *e = head;
	while(index) 
  {
		e = e->next;
		index--;
	}
	return e;
}


// ********************************************************************
// QMAP routines

void DecodeMapParams ( char* params, FILE* stream )
// decode params from entity helper.
// The entity helper object stores the free form user parameters
// as a string. The '|' character is used to separate each parameter 
// and argument.
{
  char * start; char* end ; char str [128] ; BOOL token ;

  start = params ;
  while (TRUE)
  {
    token = FALSE ;
    // arg
    end = strchr(start,'|') ;
    if ( end == NULL )
      return ;
    memset(str,0,sizeof(str)) ;
    strncpy(str,start,(end-start)) ;
    if ( str[0] != (char)0 )
    {
      fprintf(stream,"\"%s\" ",str) ;
      token = TRUE ;
    } ;
    start= end+1 ;
    // val
    end = strchr(start,'|') ;
    if ( end == NULL )
      return ;
    memset(str,0,sizeof(str)) ;
    strncpy(str,start,(end-start)) ;
    if ( str[0] != (char)0 )
    {
      fprintf(stream,"\"%s\"",str) ;
      token = TRUE ;
    } ;
    if (token)
      fprintf(stream,"\n") ;
    start= end+1 ;
  } ;
} ;

void GetMtlTextureData ( Mtl* m, int& xOff, int& yOff, float& rotAngle, float& xScale, float& yScale )
// given material, return the QMAP texture data
{
  if ( m->ClassID() == Class_ID(DMTL_CLASS_ID,0))
  {
    StdMat* std = (StdMat*)m ;
    Texmap* tx ;
    if ( (tx = std->GetSubTexmap(1)) != NULL )
    {
      // just do bitmaps
      if ( tx->ClassID() == Class_ID(BMTEX_CLASS_ID,0))
      {
        BitmapTex* bmt = (BitmapTex*)tx ;
        StdUVGen* uv = bmt->GetUVGen() ;
        xOff      = (int)uv->GetUOffs(0) ;
        yOff      = (int)uv->GetVOffs(0) ;
        xScale    = uv->GetUScl(0) ;
        yScale    = uv->GetVScl(0) ;
        rotAngle  = (uv->GetAng(0)/(2.0f*3.14159265359f))*360.0f ;
      } ;
    } ;
  } ;
} ;

int errorBrushes ;

BOOL WriteMesh ( MySceneEntry* sEntry, Interface* gi, FILE* stream )
// given a MAX object, convert and write out as a Quake brush
{
  int numPlanes = 0 ;
  // get entry from scene
  INode* n = sEntry->node ;
  Object* obj = sEntry->obj ;
  // convert object to a mesh (has been checked by MySceneEnumProc that this is possible)
  TriObject* tri = (TriObject*) obj->ConvertToType(gi->GetTime(),triObjectClassID) ;
  Mesh &mesh = tri->mesh ;
  // get transformation matrix
  Matrix3 tm = n->GetObjectTM(gi->GetTime()) ;
  // check for valid object
  if (mesh.numFaces == 0)
    return FALSE ;

  // got node, get material
  Mtl* nodeMtl = n->GetMtl() ;
  BOOL multiMtl = FALSE ;
  if ( nodeMtl != NULL)
  {
    multiMtl = nodeMtl->IsMultiMtl() ;
  } ;

  // ok, got a mesh, now convert it
  int verts = mesh.getNumVerts() ;
  int faces = mesh.getNumFaces() ;

  // start brush definition
  if (fprintf(stream,"{\n"
                 "// %s\n"
                 ,n->GetName() 
            )<0)
    return TRUE ;


  int i, j ; Point3 vert ; Point3 cross ; int numNormals = 0 ;
#define MAX_NORMALS 256
  Face face ; 
  Point3 normal     [MAX_NORMALS] ;   // list of face normals

  for ( i = 0 ; i<faces ; i++ )  // loop for all faces in the mesh
  {
    face = mesh.faces[i] ;
    Point3 vert ; 
    // find face normal by cross product
    float ax, ay, az, bx, by, bz, cx, cy, cz, u1, u2, u3, v1, v2, v3, length ;
    Point3 cross ;

    vert = mesh.verts[face.v[0]] * tm ;
    ax = vert.x ; ay = vert.y ; az = vert.z ;
    vert = mesh.verts[face.v[1]] * tm ;
    bx = vert.x ; by = vert.y ; bz = vert.z ;
    vert = mesh.verts[face.v[2]] * tm ;
    cx = vert.x ; cy = vert.y ; cz = vert.z ;

    u1 = ax - bx ; u2 = ay - by ; u3 = az - bz ;
    v1 = cx - bx ; v2 = cy - by ; v3 = cz - bz ;
    cross.x = (u2*v3) - (u3*v2) ;
    cross.y = (u3*v1) - (u1*v3) ;
    cross.z = (u1*v2) - (u2*v1) ;
    length = sqrt( (cross.x*cross.x) + (cross.y*cross.y) + (cross.z*cross.z) ) ;
    cross.x = cross.x/length ;
    cross.y = cross.y/length ;
    cross.z = cross.z/length ;

    // handle material
    char mtlName [60] ; int xOff, yOff ; float rotAngle, xScale, yScale ;
    MtlID mat = face.getMatID() ;
    // preset defualt material data
    strcpy(mtlName,"UNKNOWN") ;
    xOff = yOff = 0 ; rotAngle = 0.0f ; xScale = yScale = 1.0f ;
    if ( multiMtl )
    {
      // if multi material get sub material
      if ( nodeMtl != NULL )
      {
        Mtl* subMtl = nodeMtl->GetSubMtl(mat) ;
        if ( subMtl != NULL )
        {
          strcpy(mtlName,subMtl->GetName());
          GetMtlTextureData(subMtl,xOff,yOff,rotAngle,xScale,yScale) ;
        } ;
      } ;
    } 
    else    // not multi material
    {
      if ( nodeMtl != NULL )
      {
        strcpy(mtlName,nodeMtl->GetName()) ;
        GetMtlTextureData(nodeMtl,xOff,yOff,rotAngle,xScale,yScale) ;
      } ;
    } ;
    if ( numNormals == 0 )    // if first face, start face list
    {
      normal[0] = cross ; 
      numNormals++ ;
      // write brush plane definition
      if (fprintf(stream, "( %ld %ld %ld ) "
                          "( %ld %ld %ld ) "
                          "( %ld %ld %ld ) "
                          "%s %ld %ld %f %f %f\n",
                          (int)ax, (int)ay, (int)az,
                          (int)cx, (int)cy, (int)cz,
                          (int)bx, (int)by, (int)bz,
                          mtlName, xOff, yOff, rotAngle, xScale, yScale
                 )<0)
        return TRUE ;
      numPlanes++ ;     // count planes for error check
    }
    else      // not first face
    {
      // now scan through past faces looking for same normal
      BOOL planeFound = FALSE ;
      for ( j = 0 ; j<=numNormals ; j++ )
      {
        planeFound =  (cross.x == normal[j].x) &&
                      (cross.y == normal[j].y) &&
                      (cross.z == normal[j].z) ;
        if (planeFound)
        {
          // same normal (i.e. plane) found, now check at least one vertex matches
          planeFound = FALSE ; // assume no equal verts
          int v1 ;
          for ( v1 = 0 ; v1 < 4; v1++)
          {
            if ((mesh.faces[j].v[0] == face.v[v1]) ||
                (mesh.faces[j].v[1] == face.v[v1]) ||
                (mesh.faces[j].v[2] == face.v[v1]))
            {
              v1 = 4 ; j = numNormals ;     // exit loops
              planeFound = TRUE ;           // flag face is part of same plane 
            } ;
          } ;
        } ;
      } ;
      numNormals++ ;
      // check for too many normals
      if ( numNormals >= MAX_NORMALS )
      {
        char errStr [255] ;
        sprintf(errStr,"Error: Object %s has too many normals (i.e. faces)",n->GetName()) ;
        MessageBox(GetActiveWindow(),errStr,"QMAPEXP", MB_OK) ;
        i = faces ;     // exit loop
      } 
      else  // save normal in list
      {
        normal[numNormals] = cross ; 
        if (!planeFound)      // if no existing plane found
        {
          if (fprintf(stream, "( %ld %ld %ld ) "
                              "( %ld %ld %ld ) "
                              "( %ld %ld %ld ) "
                              "%s %ld %ld %f %f %f\n",
                              (int)ax, (int)ay, (int)az,
                              (int)cx, (int)cy, (int)cz,
                              (int)bx, (int)by, (int)bz,
                              mtlName, xOff, yOff, rotAngle, xScale, yScale
                      )<0)
            return TRUE ;
          numPlanes++ ;
        } ;
      } ;
    } ;
  } ;
  if ( numPlanes < 4 )     // check for min Quake brush
  {
    errorBrushes++ ;
    if (fprintf(stream,"// ****ERROR*** BRUSH HAS TOO FEW PLANES (<4)\n")<0)
      return TRUE ;
  } ;
  // finish brush definition
  if (fprintf(stream,"}\n")<0)
    return TRUE ;

  if ( obj != (Object*)tri)   // delete work object if needed
    tri->DeleteThis() ;  

  return FALSE ;
} ;

int qmap_save(const TCHAR *filename, ExpInterface *ei, Interface *gi, QMapExport *exp)
{

	// Put up the options dialog to find out how they want the file written!
	int result = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_EXPORTOPTIONS), 
		gi->GetMAXHWnd(), ExportOptionsDlgProc, (LPARAM)exp);
	if(result <= 0)
		return 0;

	// Make sure there are nodes we're interested in!
	// Ask the scene to enumerate all its nodes so we can determine if there are any we can use
	MySceneEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	// Any useful nodes?
	if(!myScene.Count()) 
  {
		Alert(IDS_TH_NODATATOEXPORT);
		return 1;
	} ;

  // init flags
  BOOL playerStartFound = FALSE ;   // remember if theres a player start entity
  errorBrushes = 0 ;                // remember how many brushes have errors

  // Output the file as text
	WorkFile theFile(filename,_T("w"));
	FILE *stream = theFile.Stream();
	if(!stream) 
  {
		Alert(IDS_TH_CANTCREATE);
		return -1;						// Didn't open!
	}

	// Build a bounding box for the objects we're exporting
	Box3 bbox = myScene.Bound();

	// Let's get busy!

	int ox;
	TSTR layername;
	
	// Write header stuff 

  if (fprintf(stream,"// Quake Map file exported from 3DS MAX by QMAPEXP v1.04\n")<0)
    goto wrterr ;   // well if Tom Hudson can use labels....

  // write the worldspawn entity that holds all the non-linked brushes
  if (fprintf(stream,"{\n"
                     "\"classname\" \"worldspawn\"\n"
                     "\"wad\" \"%s\"\n"
                     "\"message\" \"%s\"\n"
                     "\"worldtype\" \"%d\"\n"
                     "\"sounds\" \"1\"\n",
                     exp->WADName,
                     exp->MapName,
                     exp->WorldType
              )<0)
    goto wrterr ;
	
  // first loop looking for all suitable Quake brushes 
  for (ox = 0 ; ox<myScene.Count(); ++ox )
  {
    MySceneEntry* sEntry ;
    sEntry = myScene[ox] ;
 
    int objType ;
    objType = myScene[ox]->type ;
    if (objType == OBTYPE_MESH )      // found a mesh
    {
      // check Hierarchy
      BOOL globalMesh = TRUE ;        // assume its not attched to an entity
      INode* parent = sEntry->node->GetParentNode() ; // find parent
      if (!parent->IsRootNode())                      // parent is a real object
      {
        Object* parentObj = parent->GetObjectRef() ;  // determine parent type
        if ( (parentObj->SuperClassID() == HELPER_CLASS_ID ) &&
			       (parentObj->ClassID()      == QENTHELP_CLASS_ID)) // brush is linked to entity
          globalMesh = FALSE ;

      } ;
      if (globalMesh)   // if not attached to entity
      {
        if (WriteMesh(sEntry, gi, stream))
          goto wrterr ;
      } ;
    } ;
  } ; // for

  if (fprintf(stream,"}\n")<0)
    goto wrterr ;

  // next loop looking for all suitable Quake entities
  for (ox = 0 ; ox<myScene.Count(); ++ox )
  {
    MySceneEntry* sEntry ;
    sEntry = myScene[ox] ;
    if ( sEntry->type == OBTYPE_OMNILIGHT )   // if MAX omni light
    {
      // derive the light parameters
			INode* node = sEntry->node;
			Object* obj = sEntry->obj;
			LightObject* lt= (LightObject *)obj;
			GenLight* gl = (GenLight *)obj;
			LightState ls;
    	Interval valid;
			lt->EvalLightState(myScene.time, valid, &ls);
			Matrix3 mat = node->GetNodeTM(myScene.time);			
			Point3 pos = mat.GetRow(3);

      // issue quake light
      if (fprintf(stream, "{\n"
                          "// %s\n"
                          "\"classname\" \"light\"\n"
                          "\"origin\" \"%ld %ld %ld\"\n"
                          "\"light\" \"%d\"\n"
                          "}\n"
                          ,node->GetName()
                          ,(int)pos.x,(int)pos.y,(int)pos.z
                          ,(int)ls.attenEnd  // MAX attenuation end controls Quake intensity
          )<0)
      goto wrterr ;

    } 
    else
    {
      if ( sEntry->type == OBTYPE_QENTITY )   // quake entity helper object
      {
        INode* node = sEntry->node ;
        QEntHelpObject* ent = (QEntHelpObject*)sEntry->obj ;
			  Matrix3 mat = node->GetNodeTM(myScene.time);			
			  Point3 pos = mat.GetRow(3);

        // check if its part of a tree
        int numChild = node->NumberOfChildren() ; 
        if ( numChild == 0 )        // if unatttached, standard entity
        {
          // issue standard quake entity
          if (fprintf(stream, "{\n"
                              "// %s\n"
                              "\"classname\" \"%s\"\n"
                              "\"origin\" \"%ld %ld %ld\"\n"
                              ,node->GetName()
                              ,ent->QClassName
                              ,(int)pos.x,(int)pos.y,(int)pos.z
              )<0)
          goto wrterr ;
          // issue map params
          DecodeMapParams(ent->Data.MapParams,stream) ;
          // check if its the player start entity
          if ( strcmp(ent->QClassName,"info_player_start") == 0)
            playerStartFound = TRUE ;
          if (fprintf(stream,"}\n") <0)
            goto wrterr ;
        } 
        else      // its an entity linked to a brush
        {
          if ( strcmp(ent->QClassName,"func_door") == 0 )   // its a door
          {
            // write entity
            if (fprintf(stream, "{\n"
                                "// %s\n"
                                "\"classname\" \"%s\"\n"
                                "\"angle\" \"%ld\"\n"
                                "\"speed\" \"%ld\"\n"
                                "\"sounds\" \"%ld\"\n"
                                "\"wait\" \"%ld\"\n"
                                "\"lip\" \"%ld\"\n"
                                "\"dmg\" \"%ld\"\n"
                                ,node->GetName()
                                ,ent->QClassName
                                ,ent->Data.Angle
                                ,ent->Data.Speed
                                ,ent->Data.Sounds
                                ,ent->Data.Wait
                                ,ent->Data.Lip
                                ,ent->Data.Dmg
                )<0)
            goto wrterr ;

            if (strlen(ent->Data.TargetName) != 0)
            {
              if (fprintf(stream, "\"targetname\" \"%s\"\n"
                                  ,ent->Data.TargetName
                  )<0)
              goto wrterr ;
            } ;
            if (strlen(ent->Data.Message) != 0)
            {
              if (fprintf(stream, "\"message\" \"%s\"\n"
                                  ,ent->Data.Message
                  )<0)
              goto wrterr ;
            } ;
            if ( ent->Data.Health != 0)
            {
              if (fprintf(stream, "\"health\" \"%ld\"\n"
                                  ,ent->Data.Health
                  )<0)
              goto wrterr ;
            } ;
            if ( ent->Data.Toggle)
            {
              if (fprintf(stream, "\"toggle\"\n")<0)
              goto wrterr ;
            } ;
            if ( ent->Data.StartOpen)
            {
              if (fprintf(stream, "\"start_open\"\n")<0)
              goto wrterr ;
            } ;
            if ( ent->Data.DoorDontLink)
            {
              if (fprintf(stream, "\"door_dont_link\"\n")<0)
              goto wrterr ;
            } ;
            if ( ent->Data.GoldKey)
            {
              if (fprintf(stream, "\"gold_key\"\n")<0)
              goto wrterr ;
            } ;
            if ( ent->Data.SilverKey)
            {
              if (fprintf(stream, "\"silver_key\"\n")<0)
              goto wrterr ;
            } ;
            // issue map params
            DecodeMapParams(ent->Data.MapParams,stream) ;

            int childNum = 0;
            while ( childNum < numChild )
            {
              INode* childNode = node->GetChildNode(childNum) ;
              Object* childObj =  childNode->GetObjectRef() ;
	            if( childObj->CanConvertToType(triObjectClassID) ) 
              {
                // dummy up a temp scene object
                MySceneEntry ent(childNode,childObj,OBTYPE_MESH) ;
                // write child brush
                if (WriteMesh(&ent, gi, stream))
                  goto wrterr ;
		            
	            }
              else
              {
                char errStr [ 80 ] ;
                sprintf(errStr,"Error: Entity '%s' linked to non-mesh object",ent->QClassName) ; 
                MessageBox(GetActiveWindow(),errStr,"QMAPEXP", MB_OK) ;
              } 
              childNum ++ ;
            } ;
            if (fprintf(stream,"}\n")<0)
              goto wrterr ;
          }
          else    // unknown link - just issue params
          {
            if (fprintf(stream, "{\n"
                                "// %s\n"
                                "\"classname\" \"%s\"\n"
                                ,node->GetName()
                                ,ent->QClassName
                )<0)
            goto wrterr ;
            // issue map params
            DecodeMapParams(ent->Data.MapParams,stream) ;
            // issue linked brushes
            int childNum = 0;
            while ( childNum < numChild )
            {
              INode* childNode = node->GetChildNode(childNum) ;
              Object* childObj =  childNode->GetObjectRef() ;
              // check if its a meshable object
	            if( childObj->CanConvertToType(triObjectClassID) ) 
              {
                // dummy up a temp scene object
                MySceneEntry ent(childNode,childObj,OBTYPE_MESH) ;
                // write child brush
                if (WriteMesh(&ent, gi, stream))
                  goto wrterr ;
		            
	            }
              else
              {
                char errStr [ 80 ] ;
                sprintf(errStr,"Error: Entity '%s' linked to non-mesh object",ent->QClassName) ; 
                MessageBox(GetActiveWindow(),errStr,"QMAPEXP", MB_OK) ;
              } 
              childNum ++ ;
            } ;
            if (fprintf(stream,"}\n")<0)
              goto wrterr ;
          } ;
        } ;
      } ;
    } ;
  } ;
  // finish off file
  if (!playerStartFound)      // all maps must have a player start
  {
    MessageBox(GetActiveWindow(),"Map has no player start entity. Default start added at 10,10,10","QMAPEXP", MB_OK) ;
    if (fprintf(stream, "{\n"
                        "\"classname\"	\"info_player_start\"\n"
                        "\"origin\"	\"10 10 10\"\n"
                        "}\n"
               ) <0 )
      goto wrterr ;
  } ;


  // all done, close MAP file
	if(theFile.Close())
		goto wrterr;

  // check for bad brushes
  if ( errorBrushes != 0 )
  {
    char errStr[80] ;
    sprintf(errStr,"Warning: %d brushes have errors, see MAP file.",errorBrushes) ;
    MessageBox(GetActiveWindow(),errStr,"QMAPEXP", MB_OK) ;
  } ;

	return 1;	

wrterr:
  Alert(IDS_TH_WRITEERROR) ;
  theFile.Close() ;
  remove(filename) ;
  return 0 ;
} ;

int QMapExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi) 
{
	int status;
		
	status=qmap_save(filename, ei, gi, this);

	if(status == 0)
		return 1;		// Dialog cancelled
	if(status > 0)	{
#ifdef DBGQMAP
DebugPrint("QMAP status OK!\n");
#endif
		}
#ifdef DBGQMAP
	else
	if(status < 0)
		DebugPrint("Error somewhere in QMAP!\n");
#endif

	return(status);
} ;
