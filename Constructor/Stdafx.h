#pragma warning(disable)

// Windows
#include <windows.h>

// Stdlib
#include <iostream>
#include <stddef.h>
#include <cassert>
#include <sstream>
#include <string>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <chrono>

// GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudalegacy.hpp>

// OpenMP
#include <omp.h>

// Boost
#include <boost/serialization/vector.hpp>  
#include <boost/archive/binary_oarchive.hpp>  
#include <boost/archive/binary_iarchive.hpp> 
#include <boost/filesystem.hpp>
#include <boost/serialization/list.hpp>

// CUDA Runtime, Interop, and includes
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <vector_types.h>
#include <vector_functions.h>
#include <driver_functions.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// Zlib
#include <zlib.h>

// Kill the using keyword with fire
#define using syntaxerror

// Visual leak detector
#include <vld.h> 