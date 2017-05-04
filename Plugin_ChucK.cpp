//
//  Plugin_ChucK.cpp
//  AudioPluginDemo
//
//  Created by Jack Atherton on 4/19/17.
//
//


#include "Plugin_ChucK.h"

#include "chuck_system.h"
#include <iostream>
#include <map>

namespace ChucK
{
    enum Param
    {
        P_CHUCKID,
        P_NUM
    };

    struct EffectData
    {
        struct Data
        {
            float p[P_NUM];
            Chuck_System * chuck;
            t_CKINT myId;
            bool initialized;
        };
        union
        {
            Data data;
            unsigned char pad[(sizeof(Data) + 15) & ~15]; // This entire structure must be a multiple of 16 bytes (and and instance 16 byte aligned) for PS3 SPU DMA requirements
        };
    };
    
    std::map< unsigned int, EffectData::Data * > chuck_instances;
    
    
    void startChuck( EffectData::Data * data )
    {
        data->chuck = new Chuck_System;
        
        // equivalent to "chuck --loop --silent" on command line,
        // but without engaging the audio loop -- we'll do that
        // via the Unity callback
        std::vector< char * > argsVector;
        char arg1[] = "chuck";
        char arg2[] = "--loop";
        char arg3[] = "--silent";
        argsVector.push_back( & arg1[0] );
        argsVector.push_back( & arg2[0] );
        argsVector.push_back( & arg3[0] );
        const char ** args = (const char **) & argsVector[0];
        data->chuck->go( 3, args, FALSE, TRUE );
    }
    
    
    void quitChuck( EffectData::Data * data )
    {
        Chuck_System * chuck = data->chuck;
        // this has been moved to the destructor
        //chuck->clientPartialShutdown();
        
        delete chuck;
        data->chuck = NULL;
    }

    // C# "string" corresponds to passing char *
    UNITY_INTERFACE_EXPORT bool runChuckCode( unsigned int chuckID, const char * code )
    {
        Chuck_System * chuck = chuck_instances[chuckID]->chuck;
        return chuck->compileCode( code, "" );
    }
    

    
    // on launch, reset all ids (necessary when relaunching a lot in unity editor)
    UNITY_INTERFACE_EXPORT void cleanRegisteredChucks() {
        // delete stored data pointers
        chuck_instances.clear();
    }

    
    bool RegisterChuckData( EffectData::Data * data, const unsigned int id )
    {
        // only store if id hasn't been used
        if( chuck_instances.count( id ) > 0 )
        {
            return false;
        }
        
        // store chuck by id
        chuck_instances[id] = data;
        // store id on overall data; note we might be replacing a non-zero id
        //  in the case when unity is reusing an audio callback the next time
        //  the scene is entered.
        data->myId = id;
        
        // when the plugin callback is reused, then on even times (2nd, 4th, etc),
        // audio doesn't happen and all the cued shreds get saved and played
        // on the next time (3rd, 5th, etc)
        // this is a problem with the globals, somehow... quitChuck( data ); startChuck( data ); here doesn't help
        // also, I don't seem to get crash data :( and forcing a crash here or when about to runChuckCode doesn't tell me anything
        
        return true;
    }
    
    
    // Called when the plugin is loaded.
    void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
    {
        // Things that need to be common to ALL ChucK instances will be loaded here
    }
    
    
    // Called when the plugin is about to be unloaded.
    void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
    {
        // Things that need to be common to ALL ChucK instances will be unloaded here
        unity_exit();
        
    }
    

#if !UNITY_SPU

    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition)
    {
        int numparams = P_NUM;
        definition.paramdefs = new UnityAudioParameterDefinition[numparams];
        // float vals are: min, max, default, scale (?), scale (?)
        RegisterParameter( definition, "ChucK ID", "#", -1.0f, 256.0f, -1.0f, 1.0f, 1.0f, P_CHUCKID, "Internal ID number used to run ChucK scripts programmatically. Leave set to -1 manually.");
        return numparams;
    }


    // NOTE: CreateCallback and ReleaseCallback are called at odd times, e.g.
    // - When unity launches for the first time and when unity exits
    // - When a new effect is added
    // - When a parameter is exposed
    // - NOT when play mode is activated or deactivated
    // instantiation
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState* state)
    {
        EffectData* effectdata = new EffectData;
        memset(effectdata, 0, sizeof(EffectData));
        
        // create chuck and initialize it (without audio callback)
        startChuck( & (effectdata->data) );
        effectdata->data.myId = -1; // id not yet set
        
        state->effectdata = effectdata;
        InitParametersFromDefinitions(InternalRegisterEffectDefinition, effectdata->data.p);

        return UNITY_AUDIODSP_OK;
    }


    // deletion
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState* state)
    {
        EffectData::Data * data = &state->GetEffectData<EffectData>()->data;

        quitChuck( data );
        
        delete data;

        return UNITY_AUDIODSP_OK;
    }


    // set param (piggy-backed to store chucks by id)
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState* state, int index, float value)
    {
        EffectData::Data* data = &state->GetEffectData<EffectData>()->data;
        if (index >= P_NUM)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        
        // setting ID, time to cache the pointer to effect data
        if( index == P_CHUCKID && value >= 0.0f )
        {
            // if false( ie already registered data or id ),
            // an error will be outputted to the Unity console
            if( RegisterChuckData( data, ( unsigned int )value ) == false )
            {
                return UNITY_AUDIODSP_ERR_UNSUPPORTED;
            }
        }
        
        data->p[index] = value;
        return UNITY_AUDIODSP_OK;
    }


    // get param (not useful for anything for ChucK)
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState* state, int index, float* value, char *valuestr)
    {
        EffectData::Data* data = &state->GetEffectData<EffectData>()->data;
        if (index >= P_NUM)
            return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        if (value != NULL) {
            *value = data->p[index];
        }
        if (valuestr != NULL)
            valuestr[0] = 0;
        return UNITY_AUDIODSP_OK;
    }


    // get float buffer (not useful for anything for ChucK)
    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState* state, const char* name, float* buffer, int numsamples)
    {
        return UNITY_AUDIODSP_OK;
    }

#endif

#if !UNITY_PS3 || UNITY_SPU

#if UNITY_SPU
    EffectData  g_EffectData __attribute__((aligned(16)));
    extern "C"
#endif
    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState* state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int outchannels)
    {
        EffectData::Data* data = &state->GetEffectData<EffectData>()->data;

#if UNITY_SPU
        UNITY_PS3_CELLDMA_GET(&g_EffectData, state->effectdata, sizeof(g_EffectData));
        data = &g_EffectData.data;
#endif
        
        Chuck_System * chuck = data->chuck;

        // TODO: need to handle # channels other than just hoping they match
        // (might need to translate here between chuck # channels and unity # channels
        if( chuck->vm()->m_num_adc_channels != inchannels )
        {
            std::cout << "chuck in: " << chuck->vm()->m_num_adc_channels << " unity in: " << inchannels << std::endl;
        }
        else if( chuck->vm()->m_num_dac_channels != outchannels )
        {
            std::cout << "chuck out: " << chuck->vm()->m_num_dac_channels << " unity out: " << outchannels << std::endl;
        }
        else
        {
            chuck->run(inbuffer, outbuffer, length);
        }
        // Need to add small amount of white noise (amplitude 0.00017 is fine)
        // to prevent Unity from disabling our plugin sometimes
        for (unsigned int n = 0; n < length; n++)
        {
            for (int i = 0; i < outchannels; i++)
            {
                // multiplier:
                // 0.00017 works
                // 0.00015 does not work
                outbuffer[n * outchannels + i] += rand() * 0.00017 / RAND_MAX;
            }
        }

#if UNITY_SPU
        UNITY_PS3_CELLDMA_PUT(&g_EffectData, state->effectdata, sizeof(g_EffectData));
#endif

        return UNITY_AUDIODSP_OK;
    }

#endif
}
