#pragma once

#include "Common.h"
#include "Reconstructor.h"
#include "Camera.h"
#include "DistanceKeyer.h"
#include "Settings.h"
#include "OctreeCompressor.h"

class Processor
{
	Reconstructor &m_Reconstructor;

	Settings &m_Settings;

	OctreeCompressor *m_Compressor;

	const std::vector<Camera*> &m_Cameras;

	DistanceKeyer *m_DistanceKeyer;

	long m_NumFrames;
	int m_CurrentFrame;
	int m_PreviousFrame;

	static void OnActualFramesTrackerbarChangeWrapper(int v, void *ptr)
	{
		Processor *that = (Processor*)ptr;
		that->OnActualFramesTrackerbarChange(v);
	}

	void OnActualFramesTrackerbarChange(int v);

	void DisplayFrameForegroundMatrix(void);
public:
	Processor(Settings &settings, Reconstructor &, const std::vector<Camera*> &);
	virtual ~Processor(void);

	void ProcessForeground(Camera*);
	bool ProcessFrame(void);
	void Process(void);

	const std::vector<Camera*> &GetCameras(void) const
	{
		return this->m_Cameras;
	}

	int GetCurrentFrame(void) const
	{
		return this->m_CurrentFrame;
	}

	long GetNumberOfFrames(void) const
	{
		return this->m_NumFrames;
	}

	int GetPreviousFrame(void) const
	{
		return this->m_PreviousFrame;
	}

	Reconstructor &GetReconstructor(void) const
	{
		return this->m_Reconstructor;
	}

	void SetCurrentFrame(int currentFrame)
	{
		this->m_CurrentFrame = currentFrame;
	}

	void SetNumberOfFrames(long numberOfFrames)
	{
		this->m_NumFrames = numberOfFrames;
	}

	void SetPreviousFrame(int previousFrame)
	{
		this->m_PreviousFrame = previousFrame;
	}
};