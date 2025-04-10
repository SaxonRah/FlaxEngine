#include "AudioBackend.h"

#include "Engine/Tools/AudioTool/AudioTool.h"
/*
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Audio/AudioSettings.h"
*/

Dictionary<uint32, uint32> AudioBackend::BufferSourceMap;
Dictionary<uint32, AudioEffectChain*> AudioBackend::SourceEffectChains;
CriticalSection AudioBackend::BufferMapLock;
Dictionary<uint32, Array<uint32>> AudioBackend::BufferQueueMap;

static void ProcessAudioWithEffectChain(byte* samples, byte*& processedSamples, const AudioDataInfo& info, AudioEffectChain* chain, Array<float>& processingBuffer, Array<byte>& processedData)
{
    // Skip if no effects or invalid chain
    if (!chain || chain->GetEffects().IsEmpty())
        return;

    // Convert to float for processing
    const uint32 numSamples = info.NumSamples;
    processingBuffer.Resize(numSamples);
    AudioTool::ConvertToFloat(samples, info.BitDepth, processingBuffer.Get(), numSamples);

    // Process through effect chain
    chain->Process(processingBuffer.Get(), processingBuffer.Get(), numSamples, info);

    // Allocate buffer for processed data
    processedData.Resize(info.NumSamples * info.BitDepth / 8);

    // Convert back to original format
    if (info.BitDepth == 32)
    {
        // For float data, copy directly
        Platform::MemoryCopy(processedData.Get(), processingBuffer.Get(), processedData.Count());
    }
    else
    {
        // For integer formats, convert through int32
        Array<int32> intSamples;
        intSamples.Resize(numSamples);
        AudioTool::ConvertFromFloat(processingBuffer.Get(), intSamples.Get(), numSamples);
        AudioTool::ConvertBitDepth((byte*)intSamples.Get(), 32, processedData.Get(), info.BitDepth, info.NumSamples);
    }

    // Update the pointer to use processed data
    processedSamples = processedData.Get();
}
