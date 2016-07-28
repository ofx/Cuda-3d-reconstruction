#pragma once

#pragma warning(disable)

// Stdlib
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <thread>

// Boost
#include <boost/serialization/vector.hpp>  
#include <boost/archive/binary_oarchive.hpp>  
#include <boost/archive/binary_iarchive.hpp> 
#include <boost/filesystem.hpp>
#include <boost/serialization/list.hpp>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// OpenMP
#include <omp.h>