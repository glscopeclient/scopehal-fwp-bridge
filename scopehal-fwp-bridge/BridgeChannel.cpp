#include "bridge.h"
#include "BridgeChannel.h"

using namespace std;

BridgeChannel::BridgeChannel(int i)
{
	string base = string("FastWavePort") + to_string(i);

	size_t maxMem = 40LL * 1000LL * 1000LL;	//40M max points hard coded
	size_t fileSize = maxMem * sizeof(uint16_t) + sizeof(WaveformHeader);

	//Create waveform data mapping
	string channelFileName = base + "File";
	m_hMem = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, (DWORD)fileSize, channelFileName.c_str());
	if(nullptr == m_hMem)
	{
		printf("Failed to create mapping %s\n", channelFileName.c_str());
		abort();
	}

	//Map it
	m_header = reinterpret_cast<WaveformHeader*>(MapViewOfFile(m_hMem, FILE_MAP_ALL_ACCESS, 0, 0, 0));
	if(nullptr == m_header)
	{
		printf("Failed to map view of %s\n", channelFileName.c_str());
		abort();
	}

	m_data = reinterpret_cast<int16_t*>(m_header + 1);

	//Create mutexes
	string availName = base + "MutexDataAvailable";
	string completeName = base + "MutexProcessingComplete";
	m_dataAvailable = CreateEventA(nullptr, FALSE, FALSE, availName.c_str());
	m_processingComplete = CreateEventA(nullptr, FALSE, FALSE, completeName.c_str());
	if(!m_dataAvailable || !m_processingComplete)
	{
		printf("Failed to create mutexes\n");
		abort();
	}
}