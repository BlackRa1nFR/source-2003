
#include "c_baseentity.h"
#include "cliententitylist.h"



IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_BaseEntity, BaseEntity, CBaseEntity)
END_RECV_TABLE()



IVEngineHud *engine;


IClientLeafSystem* ClientLeafSystem()
{
	return 0;
}


C_BaseEntity::C_BaseEntity()
{
	m_vecRenderOrigin.Init();
	m_vecOrigin.Init();
	m_vecRenderAngles.Init();
}

C_BaseEntity::~C_BaseEntity()
{
}

bool C_BaseEntity::Init(int entnum)
{
	return true;
}

void C_BaseEntity::Release()
{
	delete this;
}

void C_BaseEntity::SetupBones( const matrix3x4_t &parentTransform )
{
}

void C_BaseEntity::SetupWeights( )
{
}

void C_BaseEntity::PreDataUpdate( bool bnewentity )
{
}

void C_BaseEntity::PostDataUpdate( bool bnewentity )
{
}

void C_BaseEntity::AddEntity( void )
{
}

int C_BaseEntity::entindex( void )
{
	return 0;
}

const Vector& C_BaseEntity::GetOrigin( void )
{
	static Vector x(0,0,0);
	return x;
}

const Vector& C_BaseEntity::GetAngles( void )
{
	static Vector x(0,0,0);
	return x;
}

bool C_BaseEntity::IsTransparent( void )
{
	return true;
}

const struct model_s *C_BaseEntity::GetModel( void )
{
	return 0;
}

int C_BaseEntity::DrawModel( int flags )
{
	return 0;
}

int C_BaseEntity::ComputeFxBlend( void )
{
	return 0;
}

bool C_BaseEntity::LODTest()
{
	return true;
}

int	C_BaseEntity::GetModelIndex( void )
{
	return 0;
}

int	C_BaseEntity::GetMoveType( void )
{
	return 0;
}

int	C_BaseEntity::GetSolid( void )
{
	return 0;
}

CMouthInfo *C_BaseEntity::GetMouth( void )
{
	return NULL;
}

bool C_BaseEntity::ShouldDraw()
{
	return true;
}

void C_BaseEntity::UpdatePositions( void )
{
}

void C_BaseEntity::AnimTimeChanging()
{
}

bool C_BaseEntity::Interpolate( void )
{
	return false;
}

bool C_BaseEntity::Teleported( void )
{
	return false;
}

bool C_BaseEntity::IsSubModel( void )
{
	return false;
}

void C_BaseEntity::CreateLightEffects( void )
{
}

void C_BaseEntity::MoveAimEnt( void )
{
}

void C_BaseEntity::ResetAttachmentPoints( void )
{
}

void* C_BaseEntity::operator new( size_t stAllocateBlock )
{
	return malloc( stAllocateBlock );
}

void C_BaseEntity::operator delete( void *pMem )
{
	free( pMem );
}

void C_BaseEntity::AddToThinkList()
{
}

void C_BaseEntity::RemoveFromThinkList()
{
}

void C_BaseEntity::SetNextThink(float nextThinkTime)
{
}

void C_BaseEntity::Think()
{
}

float C_BaseEntity::GetTextureAnimationStartTime()
{
	return 0;
}

void C_BaseEntity::TextureAnimationWrapped()
{
}

int C_BaseEntity::DrawBrushModel( bool sort )
{
	return 0;
}

bool C_BaseEntity::IsIdentityBrush( void )
{
	return false;
}

void C_BaseEntity::Clear( void )
{
}

void C_BaseEntity::ResetPositions( bool bnewentity )
{
}

C_Team *C_BaseEntity::GetTeam( void )
{
	return 0;
}

int C_BaseEntity::GetTeamNumber( void )
{
	return 0;
}

void C_BaseEntity::SetClientClass( ClientClass* pClientClass )
{
}

void C_BaseEntity::SetClientHandle( ClientEntityHandle_t handle )
{
}

ClientEntityHandle_t C_BaseEntity::ClientHandle() const
{
	return 0;
}

const Vector& C_BaseEntity::GetMins( void )
{
	static Vector x(0,0,0);
	return x;
}

const Vector& C_BaseEntity::GetMaxs( void )
{
	static Vector x(0,0,0);
	return x;
}

void C_BaseEntity::SetInPVS( bool inside_pvs )
{
}

bool C_BaseEntity::GetInPVS( void )
{
	return true;
}

const Vector& C_BaseEntity::GetAttachment( int number )
{
	static Vector x(0,0,0);
	return x;
}

void C_BaseEntity::PutAttachment( int number, const Vector& vecOrigin )
{
}

void C_BaseEntity::DoAnimationEvents( void )
{
}

int C_BaseEntity::GetRenderTeamNumber( void )
{
	return 0;
}
									
bool C_BaseEntity::ShouldCollide( )
{
	return true;
}

SpatialPartitionHandle_t C_BaseEntity::GetClientPartition( )
{
	return 0;
}

void C_BaseEntity::SetClientPartition( SpatialPartitionHandle_t handle )
{
}

void C_BaseEntity::ReceiveMessage( int length, void *data )
{
}

void C_BaseEntity::GetBounds( Vector& mins, Vector& maxs )
{
}

void C_BaseEntity::GetColorModulation( float* color )
{
}

const Vector& C_BaseEntity::GetRenderAngles( void )
{
	static Vector bullshit(0,0,0);
	return bullshit;
}

const Vector& C_BaseEntity::GetRenderOrigin( void )
{
	static Vector bullshit(0,0,0);
	return bullshit;
}

ClientRenderHandle_t C_BaseEntity::GetRenderHandle()
{
	return 0;
}

void C_BaseEntity::SetRenderHandle( ClientRenderHandle_t handle )
{
}

bool C_BaseEntity::ShouldCacheRenderInfo()
{
	return true;
}



