#pragma warning(disable)

// Stdlib
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <thread>

// GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

// OpenMP
#include <omp.h>

// Kill the using keyword with fire
#define using syntaxerror

// Visual leak detector
#include <vld.h> 