#pragma once

/**
	@brief Wrapper for a single channel of the bridge (connects to a single FastWavePort block instrument side)
 */
class BridgeChannel
{
public:
	BridgeChannel(int i);

	HANDLE GetDataAvailableEvent()
	{ return m_dataAvailable; }

	void SetDoneEvent()
	{ SetEvent(m_processingComplete); }

	WaveformHeader* GetHeader()
	{ return m_header; }

	int16_t* GetData()
	{ return m_data; }

protected:
	///@brief The memory mapping handle
	HANDLE m_hMem;

	///@brief The mapped header
	WaveformHeader* m_header;

	///@brief The actual sample data
	int16_t* m_data;

	///@brief Mutex for waveform data ready
	HANDLE m_dataAvailable;

	///@brief Mutex indicating data has been downloaded
	HANDLE m_processingComplete;
};