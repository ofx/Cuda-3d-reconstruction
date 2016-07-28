#include "Stdafx.h"

#include "Processor.h"
#include "Exception.h"
#include "Octree.h"

Processor::Processor(Settings &settings, Reconstructor &r, const std::vector<Camera*> &cs) : m_Reconstructor(r), m_Cameras(cs), m_Settings(settings)
{
	// Initialize distance keyer, properties are adjustable through GUI
	this->m_DistanceKeyer = new DistanceKeyer();
	this->m_DistanceKeyer->SetTreshold(44);
	this->m_DistanceKeyer->SetTolerance(7);

	// Start frame
	this->m_NumFrames = m_Cameras.front()->GetFrames();
	this->m_CurrentFrame = 0;
	this->m_PreviousFrame = -1;

	this->m_Compressor = new OctreeCompressor(settings.CompressedFileName);
}

Processor::~Processor()
{
	delete this->m_DistanceKeyer;

	delete this->m_Compressor;
}

void Processor::OnActualFramesTrackerbarChange(int v)
{
	this->m_DistanceKeyer->SetTolerance(cv::getTrackbarPos("Tolerance", "Actual frames"));
	this->m_DistanceKeyer->SetTreshold(cv::getTrackbarPos("Treshold", "Actual frames"));
	this->m_DistanceKeyer->SetGarbageTreshold(cv::getTrackbarPos("Garbage Treshold", "Actual frames"));
	this->m_DistanceKeyer->SetR(cv::getTrackbarPos("R", "Actual frames"));
	this->m_DistanceKeyer->SetG(cv::getTrackbarPos("G", "Actual frames"));
	this->m_DistanceKeyer->SetB(cv::getTrackbarPos("B", "Actual frames"));

	this->DisplayFrameForegroundMatrix();
}

bool Processor::ProcessFrame(void)
{
	static bool first = true;

	for (size_t c = 0; c < this->m_Cameras.size(); ++c)
	{
		if (this->m_CurrentFrame == this->m_PreviousFrame + 1)
		{
			this->m_Cameras[c]->SetVideoFrame(100);

			this->m_Cameras[c]->NextVideoFrame();
		}
		else if (this->m_CurrentFrame != this->m_PreviousFrame)
		{
			this->m_Cameras[c]->GetVideoFrame(this->m_CurrentFrame);
		}
		else
		{
			return false;
		}

		assert(this->m_Cameras[c] != nullptr);
		this->ProcessForeground(this->m_Cameras[c]);
	}

	// If this is the first frame we want to adjust the keyer settings if we're using the keyer
	if (first && !(this->m_Settings.UseMatteStill || this->m_Settings.UseMatteVideo))
	{
		cv::namedWindow("Actual frames");

		cv::createTrackbar("Tolerance", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);
		cv::createTrackbar("Treshold", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);
		cv::createTrackbar("Garbage Treshold", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);
		cv::createTrackbar("R", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);
		cv::createTrackbar("G", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);
		cv::createTrackbar("B", "Actual frames", 0, 255, Processor::OnActualFramesTrackerbarChangeWrapper, this);

		cv::setTrackbarPos("Tolerance", "Actual frames", this->m_DistanceKeyer->GetTolerance());
		cv::setTrackbarPos("Treshold", "Actual frames", this->m_DistanceKeyer->GetTreshold());
		cv::setTrackbarPos("Garbage Treshold", "Actual frames", this->m_DistanceKeyer->GetGarbageTreshold());
		cv::setTrackbarPos("R", "Actual frames", this->m_DistanceKeyer->GetR());
		cv::setTrackbarPos("G", "Actual frames", this->m_DistanceKeyer->GetG());
		cv::setTrackbarPos("B", "Actual frames", this->m_DistanceKeyer->GetB());

		this->DisplayFrameForegroundMatrix();
		cv::waitKey(0);

		first = false;
	}

	return true;
}

void Processor::DisplayFrameForegroundMatrix(void)
{
	// Display frames
	int i = 0;
	std::vector<cv::Mat> frames;
	std::vector<cv::Mat> foregrounds;
	std::vector<Camera*>::const_iterator it;
	for (it = this->m_Cameras.begin() ; it != this->m_Cameras.end() ; ++it)
	{
		// Process foreground using current settings
		this->ProcessForeground(*it);

		cv::cuda::GpuMat mat = (*it)->GetFrame(), foreground = (*it)->GetForegroundImage();
		cv::Mat hostMat, hostForeground;
		mat.download(hostMat);
		foreground.download(hostForeground);

		frames.push_back(hostMat);
		foregrounds.push_back(hostForeground);
	}

	cv::Mat frameCanvas = Common::MakeCanvas(frames, 1080, 3);
	cv::Mat foregroundCanvas = Common::MakeCanvas(foregrounds, 1080, 3);

	std::vector<cv::Mat> m;
	m.push_back(frameCanvas);
	m.push_back(foregroundCanvas);

	cv::Mat canvas = Common::MakeCanvas(m, 1080, 1);
	cv::imshow("Actual frames", canvas);
}

void Processor::ProcessForeground(Camera *camera)
{
	if (this->m_Settings.UseMatteStill)
	{
		camera->LoadForegroundFromMatteStill();
	}
	else if (this->m_Settings.UseMatteVideo)
	{
		camera->LoadForegroundFromMatteVideo();
	}
	else
	{
		assert(!camera->GetFrame().empty());

		cv::cuda::GpuMat frame = camera->GetFrame();

		// Check if we should determine the key color
		if (!this->m_DistanceKeyer->HasKeyColor())
		{
			this->m_DistanceKeyer->FindKeyColor(frame);
		}

		cv::cuda::GpuMat gpuMatte;
		this->m_DistanceKeyer->ComputeMatte(frame, gpuMatte);

		camera->SetForegroundImage(gpuMatte);
	}
}

void Processor::Process(void)
{
	for (int n = 0 ; n < 1 && this->ProcessFrame() ; ++n)
	{
		std::cout << "Computing visible voxels..." << std::endl;

		// Update the visible voxels
		this->m_Reconstructor.Update();

		std::cout << "Computed visible voxels" << std::endl;

		const unsigned int numVisibleVoxels = this->m_Reconstructor.GetNumVisibleVoxels();
		VisibleVoxel* visibleVoxels = this->m_Reconstructor.GetVisibleVoxels();

		std::cout << "Number of visible voxels: " << numVisibleVoxels << std::endl;
		std::cout << "Memory usage: " << (numVisibleVoxels * sizeof(VisibleVoxel)) / 1000000 << "MB" << std::endl;

		std::cout << "Determining boundaries..." << std::endl;

		// Find bounds
		glm::vec3 min = {0.0f, 0.0f, 0.0f};
		glm::vec3 max = {0.0f, 0.0f, 0.0f};
		glm::vec3 cellSize = {1.0f, 1.0f, 1.0f};
		for (int i = 0 ; i < numVisibleVoxels ; ++i)
		{
			VisibleVoxel &v = visibleVoxels[i];

			min[0] = v.X < min[0] ? v.X : min[0]; max[0] = v.X > max[0] ? v.X : max[0];
			min[1] = v.X < min[1] ? v.X : min[1]; max[1] = v.X > max[1] ? v.X : max[1];
			min[2] = v.X < min[2] ? v.X : min[2]; max[2] = v.X > max[2] ? v.X : max[2];
		}

		std::cout << "Boundaries are: (" << min[0] << ", " << min[1] << ", " << min[2] << ") -> (" << max[0] << ", " << max[1] << ", " << max[2] << ")" << std::endl;

		std::cout << "Adding voxels to octree..." << std::endl;

		Octree *octree = new Octree(glm::vec3(0, 0, 0), max);
		octree->SetVoxels(visibleVoxels);
		octree->SetNumVoxels(numVisibleVoxels);

		std::cout << "Building octree..." << std::endl;

		std::chrono::system_clock::time_point start = std::chrono::high_resolution_clock::now();
		octree->Build();
		std::chrono::duration<float> b = std::chrono::system_clock::now().time_since_epoch();
		std::chrono::system_clock::time_point end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - start;
		std::cout << "Spent " << diff.count() * 1000 << " milliseconds" << std::endl;

		std::cout << "Number of nodes: " << octree->GetNumNodes() << std::endl;
		std::cout << "Memory usage: " << float(octree->GetNumNodes() * sizeof(OctreeNode)) / 1000000 << "MB" << std::endl;

		std::cout << "Compressing..." << std::endl;
		this->m_Compressor->Compress(octree);

		delete octree;
		
		std::cout << "Done, next frame!" << std::endl;
	}

	std::cout << "Done processing!" << std::endl;
}