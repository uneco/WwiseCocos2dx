#include "Wwise.h"

#include "Platform.h"

#include <AK/Plugin/AllPluginsRegistrationHelpers.h>	// Plug-ins
#ifdef AK_MOTION
#include <AK/MotionEngine/Common/AkMotionEngine.h>	// Motion Engine (required only for playback of Motion objects)
#endif // AK_MOTION

static const AkUInt32 kMaxNumPools            = 20;
static const AkUInt32 kDefaultPoolSize        = 2*1024*1024;
static const AkUInt32 kLEngineDefaultPoolSize = 1*1024*1024;


Wwise& Wwise::Instance()
{
    static Wwise wwise;
    return wwise;
}

Wwise::Wwise() {
    m_pLowLevelIO = new CAkFilePackageLowLevelIOBlocking();
}

Wwise::~Wwise()
{
    delete m_pLowLevelIO;
    m_pLowLevelIO = NULL;
}

CAkFilePackageLowLevelIOBlocking& Wwise::IOManager()
{
    return *m_pLowLevelIO;
}

bool Wwise::Init(
                 AkMemSettings&          in_memSettings,
                 AkStreamMgrSettings&    in_stmSettings,
                 AkDeviceSettings&       in_deviceSettings,
                 AkInitSettings&         in_initSettings,
                 AkPlatformInitSettings& in_platformInitSettings,
                 AkMusicSettings&        in_musicInit,
                 AkOSChar*               in_soundBankPath,
                 AkOSChar*               in_szErrorBuffer,
                 unsigned int            in_unErrorBufferCharCount
                 )
{
    bool bSuccess;
    
    // Initialize Wwise
    bSuccess = InitWwise( in_memSettings, in_stmSettings, in_deviceSettings, in_initSettings, in_platformInitSettings, in_musicInit, in_szErrorBuffer, in_unErrorBufferCharCount );
    if ( !bSuccess )
    {
        goto cleanup;
    }
    
    // Set the path to the SoundBank Files.
    m_pLowLevelIO->SetBasePath( in_soundBankPath );
    
    // Set global language. Low-level I/O devices can use this string to find language-specific assets.
    if ( AK::StreamMgr::SetCurrentLanguage( AKTEXT( "English(US)" ) ) != AK_Success )
    {
        goto cleanup;
    }
    
    return true;
    
cleanup:
    Term();
    return false;
}

void Wwise::Term()
{
    TermWwise();
}

void Wwise::GetDefaultSettings(AkMemSettings&          out_memSettings,
                               AkStreamMgrSettings&    out_stmSettings,
                               AkDeviceSettings&       out_deviceSettings,
                               AkInitSettings&         out_initSettings,
                               AkPlatformInitSettings& out_platformInitSettings,
                               AkMusicSettings&        out_musicInit)
{
    out_memSettings.uMaxNumPools = kMaxNumPools;
    AK::StreamMgr::GetDefaultSettings( out_stmSettings );
    
    AK::StreamMgr::GetDefaultDeviceSettings( out_deviceSettings );
    
    AK::SoundEngine::GetDefaultInitSettings( out_initSettings );
    out_initSettings.uDefaultPoolSize = kDefaultPoolSize;
    
    AK::SoundEngine::GetDefaultPlatformInitSettings( out_platformInitSettings );
    out_platformInitSettings.uLEngineDefaultPoolSize = kLEngineDefaultPoolSize;
    
    AK::MusicEngine::GetDefaultInitSettings( out_musicInit );
}

const bool Wwise::GetCommunicationEnabled() {
#if !defined AK_OPTIMIZED && !defined INTEGRATIONDEMO_DISABLECOMM
    return true;
#else
    return false;
#endif
}

bool Wwise::InitWwise(
                      AkMemSettings&          in_memSettings,
                      AkStreamMgrSettings&    in_stmSettings,
                      AkDeviceSettings&       in_deviceSettings,
                      AkInitSettings&         in_initSettings,
                      AkPlatformInitSettings& in_platformInitSettings,
                      AkMusicSettings&        in_musicInit,
                      AkOSChar*               in_szErrorBuffer,
                      unsigned int            in_unErrorBufferCharCount
                      )
{
    //
    // Create and initialize an instance of the default memory manager. Note
    // that you can override the default memory manager with your own. Refer
    // to the SDK documentation for more information.
    //
    
    AKRESULT res = AK::MemoryMgr::Init( &in_memSettings );
    if ( res != AK_Success )
    {
        __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::MemoryMgr::Init() returned AKRESULT %d"), res );
        return false;
    }
    
    //
    // Create and initialize an instance of the default streaming manager. Note
    // that you can override the default streaming manager with your own. Refer
    // to the SDK documentation for more information.
    //
    
    // Customize the Stream Manager settings here.
    
    if ( !AK::StreamMgr::Create( in_stmSettings ) )
    {
        AKPLATFORM::SafeStrCpy( in_szErrorBuffer, AKTEXT( "AK::StreamMgr::Create() failed" ), in_unErrorBufferCharCount );
        return false;
    }
    
    //
    // Create a streaming device with blocking low-level I/O handshaking.
    // Note that you can override the default low-level I/O module with your own. Refer
    // to the SDK documentation for more information.
    //
    
    // CAkFilePackageLowLevelIOBlocking::Init() creates a streaming device
    // in the Stream Manager, and registers itself as the File Location Resolver.
    res = m_pLowLevelIO->Init( in_deviceSettings );
    if ( res != AK_Success )
    {
        __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("m_lowLevelIO.Init() returned AKRESULT %d"), res );
        return false;
    }
    
    //
    // Create the Sound Engine
    // Using default initialization parameters
    //
    
    res = AK::SoundEngine::Init( &in_initSettings, &in_platformInitSettings );
    if ( res != AK_Success )
    {
        __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::SoundEngine::Init() returned AKRESULT %d"), res );
        return false;
    }
    
    //
    // Initialize the music engine
    // Using default initialization parameters
    //
    
    res = AK::MusicEngine::Init( &in_musicInit );
    if ( res != AK_Success )
    {
        __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::MusicEngine::Init() returned AKRESULT %d"), res );
        return false;
    }
    
    //
    // Initialize communications (not in release build!)
    //
    if (GetCommunicationEnabled()) {
        AkCommSettings commSettings;
        AK::Comm::GetDefaultInitSettings( commSettings );
        res = AK::Comm::Init( commSettings );
        if ( res != AK_Success )
        {
            __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible."), res );
        }
    }
    
    //
    // Register plugins
    /// Note: This a convenience method for rapid prototyping.
    /// To reduce executable code size register/link only the plug-ins required by your game
    res = AK::SoundEngine::RegisterAllPlugins();
    if ( res != AK_Success )
    {
        __AK_OSCHAR_SNPRINTF( in_szErrorBuffer, in_unErrorBufferCharCount, AKTEXT("AK::SoundEngine::RegisterAllPlugins() returned AKRESULT %d."), res );
    }
    
    return true;
}

void Wwise::TermWwise()
{
    // Terminate communications between Wwise and the game
    if (GetCommunicationEnabled()) {
        AK::Comm::Term();
    }

    // Terminate the music engine
    AK::MusicEngine::Term();
    
    // Terminate the sound engine
    if ( AK::SoundEngine::IsInitialized() )
    {
        AK::SoundEngine::Term();
    }
    
    // Terminate the streaming device and streaming manager
    // CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device 
    // that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
    if ( AK::IAkStreamMgr::Get() )
    {
        m_pLowLevelIO->Term();
        AK::IAkStreamMgr::Get()->Destroy();
    }
    
    // Terminate the Memory Manager
    if ( AK::MemoryMgr::IsInitialized() )
    {
        AK::MemoryMgr::Term();
    }
}
